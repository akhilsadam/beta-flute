#!/usr/bin/env python3
"""
QG Wavetable Server: Runs QG simulation and streams waveform data to JUCE plugin via socket.
Generates audio-rate waveforms from QG flow by sampling at configurable x-y positions.

Install QG package: pip install git+https://github.com/akhilsadam/qg.git
"""

import socket
import struct
import threading
import time
import argparse
from pathlib import Path
import sys

import torch
import numpy as np

# QG package installed from GitHub
from qg.solver.qg import QG
from qg.solver.opt.basis import to_physical
import qg.config as vc
from omegaconf import OmegaConf


class QGWavetableServer:
    def __init__(self, config_path=None, waveform_size=2048,
                 restart_interval=5.0, device='cuda'):
        self.waveform_size = waveform_size
        self.restart_interval = restart_interval
        self.device = device
        self.running = False
        self.reinit_on_note = True  # Reinit per MIDI note by default

        # Sampling position (will be updated by plugin)
        self.sample_x = None
        self.sample_y = None
        self.lock = threading.Lock()

        # Load or create minimal config
        if config_path and Path(config_path).exists():
            cfg = OmegaConf.load(config_path)
        else:
            cfg = OmegaConf.create({
                'grid': {'Nx': 256, 'Ny': 256, 'Lx': 1.0, 'Ly': 1.0},
                'time': {'T': 10.0, 'dt': 0.01, 'save_rate': 1},
                'pde': {'nu': 1e-5, 'mu': 0.0, 'beta': 0.0},
                'integrator': {'split_bc': False, 'order': 2},
                'ic': 'random',
                'flow': {'type': 'none'},
            })

        import logging
        logger = logging.getLogger('qg_server')
        logger.setLevel(logging.INFO)

        self.qg = QG(cfg, logger=logger)
        self.state = self.qg.init()
        self.last_restart = time.time()

        # Default to center position
        self.sample_x = self.qg.grid.Nx // 2
        self.sample_y = self.qg.grid.Ny // 2

        print(f"QG initialized: {self.qg.grid.Nx}x{self.qg.grid.Ny} grid on {device}")

    def extract_waveform(self):
        """Extract a 1D waveform from the current QG state at sample position."""
        q = to_physical(self.state.qh)[0]  # First batch

        with self.lock:
            x, y = self.sample_x, self.sample_y

            # Clamp to valid range
            x = np.clip(x, 0, q.shape[1] - 1)
            y = np.clip(y, 0, q.shape[0] - 1)

        # Extract a 1D line through the sampling point
        # Take horizontal line at sample_y
        waveform = q[y, :].cpu().numpy()

        # Resample to waveform_size
        if len(waveform) != self.waveform_size:
            indices = np.linspace(0, len(waveform) - 1, self.waveform_size)
            waveform = np.interp(indices, np.arange(len(waveform)), waveform)

        # Normalize
        waveform = waveform.astype(np.float32)
        max_val = np.abs(waveform).max()
        if max_val > 0:
            waveform /= max_val

        return waveform

    def command_handler(self, sock):
        """Handle incoming commands from JUCE plugin (config, sample position, etc)."""
        sock.settimeout(0.1)
        cmd_buffer = ""

        while self.running:
            try:
                data = sock.recv(256).decode('utf-8', errors='ignore')
                if not data:
                    break

                cmd_buffer += data

                # Process complete commands (newline-terminated)
                while '\n' in cmd_buffer:
                    line, cmd_buffer = cmd_buffer.split('\n', 1)
                    line = line.strip()

                    if line.startswith("POS"):
                        # POS x y
                        parts = line.split()
                        if len(parts) == 3:
                            try:
                                x, y = int(parts[1]), int(parts[2])
                                with self.lock:
                                    self.sample_x = x
                                    self.sample_y = y
                                sock.sendall(b"OK\n")
                            except:
                                sock.sendall(b"ERR\n")

                    elif line.startswith("CONFIG"):
                        # CONFIG key value (would require runtime config update - simplified here)
                        sock.sendall(b"OK\n")

                    elif line == "REINIT_ON_NOTE":
                        # Toggle reinit on note
                        self.reinit_on_note = not self.reinit_on_note
                        state_str = "ON" if self.reinit_on_note else "OFF"
                        sock.sendall(f"REINIT_ON_NOTE {state_str}\n".encode())

                    elif line == "NOTE":
                        # Force reinit on note trigger
                        if self.reinit_on_note:
                            self.state = self.qg.init()
                            self.last_restart = time.time()
                            sock.sendall(b"OK\n")
                        else:
                            sock.sendall(b"SKIP\n")

                    elif line == "PING":
                        sock.sendall(b"PONG\n")

            except socket.timeout:
                pass
            except:
                break

    def simulation_loop(self, sock, cmd_sock):
        """Run QG simulation and stream waveforms to socket."""
        self.running = True
        frame_count = 0

        try:
            while self.running:
                # Simulate one step
                self.qg.step(self.state)
                frame_count += 1

                # Restart periodically
                if time.time() - self.last_restart > self.restart_interval:
                    self.state = self.qg.init()
                    self.last_restart = time.time()
                    print(f"[{frame_count}] Restarted QG simulation")

                # Extract waveform
                waveform = self.extract_waveform()

                # Send to JUCE: [magic:4][size:4][data:size*4]
                magic = b'QGWT'
                data_bytes = waveform.tobytes()
                size = len(waveform)

                try:
                    sock.sendall(magic)
                    sock.sendall(struct.pack('<I', size))
                    sock.sendall(data_bytes)
                except (BrokenPipeError, ConnectionResetError):
                    print("Client disconnected")
                    return

                if frame_count % 100 == 0:
                    print(f"[{frame_count}] Streamed {size} samples @ ({self.sample_x}, {self.sample_y})")

                # Control simulation speed (~100 fps)
                time.sleep(0.01)

        except Exception as e:
            print(f"Error in simulation loop: {e}")
        finally:
            self.running = False

    def start_server(self, port=9999):
        """Start socket server with separate waveform and command channels."""
        import platform
        use_tcp = platform.system() == "Windows"

        if use_tcp:
            # Windows: use TCP/IP
            wave_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            wave_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            wave_sock.bind(("127.0.0.1", port))
            wave_sock.listen(1)

            cmd_sock_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            cmd_sock_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            cmd_sock_server.bind(("127.0.0.1", port - 1))
            cmd_sock_server.listen(1)

            print(f"QG Wavetable Server listening on (TCP/Windows):")
            print(f"  Waveforms: 127.0.0.1:{port}")
            print(f"  Commands:  127.0.0.1:{port - 1}")
        else:
            # Unix/Linux/macOS: use Unix sockets
            waveform_path = f'/tmp/qg_wavetable_wave_{port}.sock'
            Path(waveform_path).unlink(missing_ok=True)

            wave_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            wave_sock.bind(waveform_path)
            wave_sock.listen(1)

            # Command socket
            cmd_path = f'/tmp/qg_wavetable_cmd_{port}.sock'
            Path(cmd_path).unlink(missing_ok=True)

            cmd_sock_server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            cmd_sock_server.bind(cmd_path)
            cmd_sock_server.listen(1)

            print(f"QG Wavetable Server listening on (Unix sockets):")
            print(f"  Waveforms: {waveform_path}")
            print(f"  Commands:  {cmd_path}")

        try:
            cmd_conn = None

            while True:
                print("Waiting for JUCE plugin connection...")
                wave_conn, _ = wave_sock.accept()
                print("Plugin connected (waveform stream)!")

                # Wait for command connection in background
                def accept_cmd():
                    nonlocal cmd_conn
                    cmd_conn, _ = cmd_sock_server.accept()
                    print("Plugin connected (command channel)!")

                cmd_thread = threading.Thread(target=accept_cmd, daemon=True)
                cmd_thread.start()

                self.simulation_loop(wave_conn, None)
                wave_conn.close()

                if cmd_conn:
                    cmd_conn.close()

        except KeyboardInterrupt:
            print("\nShutting down...")
        finally:
            wave_sock.close()
            cmd_sock_server.close()
            if not use_tcp:
                Path(waveform_path).unlink(missing_ok=True)
                Path(cmd_path).unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description='QG Wavetable Server for JUCE')
    parser.add_argument('--config', default=None, help='QG config file (optional)')
    parser.add_argument('--waveform-size', type=int, default=2048, help='Waveform samples')
    parser.add_argument('--restart-interval', type=float, default=5.0, help='Restart sim every N seconds')
    parser.add_argument('--device', default='cuda', help='torch device (cuda/cpu)')
    parser.add_argument('--port', type=int, default=9999, help='Socket port')

    args = parser.parse_args()

    server = QGWavetableServer(
        config_path=args.config,
        waveform_size=args.waveform_size,
        restart_interval=args.restart_interval,
        device=args.device
    )
    server.start_server(args.port)


if __name__ == '__main__':
    main()


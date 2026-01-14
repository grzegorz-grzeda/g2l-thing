import argparse
import subprocess

BOARD = 'esp32c6_devkitc'
SERIAL_PORT = '/dev/ttyACM1'


def parse_args():
    parser = argparse.ArgumentParser(
        description="Build and flash g2l-thing firmware"
    )
    subparsers = parser.add_subparsers(
        dest='command', required=True, help='Available commands')

    build_parser = subparsers.add_parser('build', help='Build the firmware')
    build_parser.add_argument('--board', default=BOARD,
                              help='Target board for the build')
    build_parser.add_argument(
        '--clean', action='store_true', help='Clean build directory before building')

    flash_parser = subparsers.add_parser(
        'flash', help='Flash the firmware to the board')
    flash_parser.add_argument(
        '--port', default=SERIAL_PORT, help='Serial port of the board')
    flash_parser.add_argument(
        '-m', '--monitor', action='store_true', help='Start monitor after flashing')

    monitor_parser = subparsers.add_parser(
        'monitor', help='Monitor output from the board')
    monitor_parser.add_argument(
        '--port', default=SERIAL_PORT, help='Serial port of the board')

    return parser.parse_args()


def main():
    args = parse_args()

    if args.command == 'build':
        print(f"Building firmware for board {args.board}...")
        if args.clean:
            subprocess.run(['west', 'build', '--pristine',
                           '-b', args.board, 'application'])
        else:
            subprocess.run(['west', 'build'])
    elif args.command == 'flash':
        subprocess.run(['west', 'flash', '--esp-device', args.port])
        if args.monitor:
            subprocess.run(['west', 'espressif', 'monitor', '-p', args.port])
    elif args.command == 'monitor':
        subprocess.run(['west', 'espressif', 'monitor', '-p', args.port])


if __name__ == '__main__':
    main()

import argparse
import subprocess

BOARD = 'esp32c6_devkitc/esp32c6/hpcore'
SNIPPETS = []
# SNIPPETS = ['flash-16M']
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


def build_target(board, clean):
    print(f"Building {'clean' if clean else ''} firmware for board '{board}'")
    build_cmd = ['west', 'build']
    if clean:
        build_cmd.append('--pristine')
        build_cmd.append('--sysbuild')
        build_cmd.extend(['-b', board, 'application'])
        if SNIPPETS:
            build_cmd.extend(['-S', ' -S '.join(SNIPPETS)])

    subprocess.run(build_cmd, check=True)


def main():
    args = parse_args()

    if args.command == 'build':
        build_target(args.board, args.clean)
    elif args.command == 'flash':
        subprocess.run(['west', 'flash', '--esp-device', args.port])
        if args.monitor:
            subprocess.run(['west', 'espressif', 'monitor',
                           '-p', args.port], check=True)
    elif args.command == 'monitor':
        subprocess.run(['west', 'espressif', 'monitor',
                       '-p', args.port], check=True)


if __name__ == '__main__':
    main()

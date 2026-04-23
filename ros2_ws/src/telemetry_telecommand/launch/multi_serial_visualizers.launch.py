from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    fc_tm_port = LaunchConfiguration("fc_tm_port")
    c_tc_port = LaunchConfiguration("c_tc_port")
    l_tc_port = LaunchConfiguration("l_tc_port")
    baud_rate = LaunchConfiguration("baud_rate")
    timeout_ms = LaunchConfiguration("timeout_ms")
    crc8_variant = LaunchConfiguration("crc8_variant")
    destination_ids = LaunchConfiguration("destination_ids")
    handled_frame_types = LaunchConfiguration("handled_frame_types")

    return LaunchDescription([
        DeclareLaunchArgument(
            "fc_tm_port",
            default_value="/dev/ttyS7",
            description="FC_TM serial port for fc_tm_serial_recv.",
        ),
        DeclareLaunchArgument(
            "c_tc_port",
            default_value="/dev/ttyS3",
            description="C_TC serial port for c_tc_serial_recv.",
        ),
        DeclareLaunchArgument(
            "l_tc_port",
            default_value="/dev/ttyS4",
            description="L_TC serial port for l_tc_serial_recv.",
        ),
        DeclareLaunchArgument(
            "baud_rate",
            default_value="115200",
            description="Baud rate applied to all three serial receiver nodes.",
        ),
        DeclareLaunchArgument(
            "timeout_ms",
            default_value="100",
            description="Read timeout in milliseconds for all three serial receiver nodes.",
        ),
        DeclareLaunchArgument(
            "crc8_variant",
            default_value="crc8",
            description="CRC-8 variant used by all three serial receiver nodes.",
        ),
        DeclareLaunchArgument(
            "destination_ids",
            default_value="",
            description=(
                "Comma-separated destination UAV IDs accepted by all receivers. "
                "Empty means any destination ID."
            ),
        ),
        DeclareLaunchArgument(
            "handled_frame_types",
            default_value="0C,0D,10,11",
            description="Comma-separated frame types handled by all receivers, interpreted as hex bytes.",
        ),
        Node(
            package="telemetry_telecommand",
            executable="fc_tm_serial_recv",
            name="fc_tm_serial_recv",
            output="screen",
            parameters=[{
                "port": fc_tm_port,
                "baud_rate": baud_rate,
                "timeout_ms": timeout_ms,
                "crc8_variant": crc8_variant,
                "destination_ids": destination_ids,
                "handled_frame_types": handled_frame_types,
            }],
        ),
        Node(
            package="telemetry_telecommand",
            executable="c_tc_serial_recv",
            name="c_tc_serial_recv",
            output="screen",
            parameters=[{
                "port": c_tc_port,
                "baud_rate": baud_rate,
                "timeout_ms": timeout_ms,
                "crc8_variant": crc8_variant,
                "destination_ids": destination_ids,
                "handled_frame_types": handled_frame_types,
            }],
        ),
        Node(
            package="telemetry_telecommand",
            executable="l_tc_serial_recv",
            name="l_tc_serial_recv",
            output="screen",
            parameters=[{
                "port": l_tc_port,
                "baud_rate": baud_rate,
                "timeout_ms": timeout_ms,
                "crc8_variant": crc8_variant,
                "destination_ids": destination_ids,
                "handled_frame_types": handled_frame_types,
            }],
        ),
        Node(
            package="telemetry_telecommand",
            executable="frame_visualizer",
            name="fc_tm_frame_visualizer",
            output="screen",
            parameters=[{
                "topic": "fc_tm_synced_frame",
            }],
        ),
        Node(
            package="telemetry_telecommand",
            executable="frame_visualizer",
            name="c_tc_frame_visualizer",
            output="screen",
            parameters=[{
                "topic": "c_tc_synced_frame",
            }],
        ),
        Node(
            package="telemetry_telecommand",
            executable="frame_visualizer",
            name="l_tc_frame_visualizer",
            output="screen",
            parameters=[{
                "topic": "l_tc_synced_frame",
            }],
        ),
    ])

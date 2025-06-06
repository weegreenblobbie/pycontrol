import socket

def send_message(message, target_ip, target_port):
    """
    Sends a UDP message (text or bytes) to a specified IP address and port.

    Args:
        message (str or bytes): The message to send. If a string, it will be
                                encoded to UTF-8 bytes before sending.
        target_ip (str): The IP address of the destination.
        target_port (int): The UDP port of the destination.
    """
    # Create a UDP socket
    # AF_INET specifies the address family (IPv4)
    # SOCK_DGRAM specifies the socket type (UDP)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        # Encode the message to bytes if it's a string
        if isinstance(message, str):
            encoded_message = message.encode('utf-8')
        elif isinstance(message, bytes):
            encoded_message = message
        else:
            raise TypeError("Message must be a string or bytes.")

        # Send the message
        # sendto() is used for UDP because it includes the destination address
        sock.sendto(encoded_message, (target_ip, target_port))
        print(f"UDP message sent to {target_ip}:{target_port}: '{message}'")

    except socket.gaierror as e:
        print(f"Error resolving host {target_ip}: {e}")
    except socket.error as e:
        print(f"Socket error: {e}")
    except TypeError as e:
        print(f"Type error: {e}")
    finally:
        # Close the socket
        sock.close()

#!/bin/bash

# Define the file to remove
file_to_remove="/run/nologin"

# Remove the file if it exists
if [ -f "$file_to_remove" ]; then
    echo "Removing $file_to_remove..."
    sudo rm -f "$file_to_remove"
fi

# Start SSH daemon
echo "Starting SSH daemon..."
sudo bash -c "nohup /usr/sbin/sshd &"
echo "Done"

### Configure keys here?
base_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
### Configure keys here?
base_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ -f "$base_path/keys/id_rsa" ] && [ -f "$base_path/keys/id_rsa.pub" ]; then
    ### copy private key
    mkdir -p $HOME/.ssh
    cp $base_path/keys/id_rsa $HOME/.ssh

    ### install public key
    cp $base_path/keys/id_rsa.pub $HOME/.ssh
    cp $base_path/keys/id_rsa.pub $HOME/.ssh/authorized_keys

    chown -R astrisw:astrima $HOME/.ssh
    chmod -R 600 $HOME/.ssh
    echo "Keys installed in $HOME/.ssh"
else
    echo "Private or public key file does not exist. Skipping keys configuration."
fi


# Execute the CMD or specified command
tail -f /dev/null


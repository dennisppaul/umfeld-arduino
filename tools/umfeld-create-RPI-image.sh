#!/bin/zsh

# Check for arguments
if [[ -z "$1" || -z "$2" ]]; then
  echo "Usage: $0 <device path> <output directory>"
  echo "Example: $0 /dev/disk3 ~/rpi-backups"
  exit 1
fi

SD_DEVICE="$1"
OUTPUT_DIR="$2"
DATE_STR=$(date +%Y-%m-%d)
IMAGE_NAME="umfeld-rpi-$DATE_STR.img"
IMAGE_PATH="$OUTPUT_DIR/$IMAGE_NAME"
COMPRESSED_PATH="$IMAGE_PATH.gz"

# Check if device exists
if [[ ! -e "$SD_DEVICE" ]]; then
  echo "❌ Device $SD_DEVICE does not exist."
  exit 1
fi

# Confirm with user
echo "⚠️  This will create a full image of $SD_DEVICE"
read "confirm?Are you sure this is correct? (y/n): "
if [[ "$confirm" != "y" ]]; then
  echo "Aborted."
  exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Unmount partitions if necessary (macOS-specific)
echo "Unmounting $SD_DEVICE..."
diskutil unmountDisk "$SD_DEVICE" || {
  echo "❌ Failed to unmount $SD_DEVICE. Abort."
  exit 1
}

# Convert /dev/diskX to /dev/rdiskX for raw read (much faster)
RAW_DEVICE="${SD_DEVICE/disk/rdisk}"

# Create image
echo "Creating image from $RAW_DEVICE..."
sudo dd if="$RAW_DEVICE" of="$IMAGE_PATH" bs=4m status=progress conv=sync

# Compress image
echo "Compressing image..."
gzip "$IMAGE_PATH"

# Done
echo "✅ Backup complete: $COMPRESSED_PATH"
echo "You can now flash it using Raspberry Pi Imager or:"
echo "    sudo dd if=$COMPRESSED_PATH of=/dev/rdiskX bs=4m"

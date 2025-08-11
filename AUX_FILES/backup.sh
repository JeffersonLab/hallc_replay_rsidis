#!/usr/bin/env bash

# Configuration
SRC_FILE="rsidis_runlist.dat"
DEST_DIR="BACKUP_runlists"

# Ensure source file exists
if [[ ! -f "$SRC_FILE" ]]; then
  echo "Error: Source file '$SRC_FILE' not found in current directory."
  exit 1
fi

# Create destination directory if it doesn't exist
mkdir -p "$DEST_DIR"

# Generate date prefix (MM_DD)
DATE_STR=$(date +%m_%d)

# Determine next version number for today
# Count files in DEST_DIR matching today's prefix and the pattern *_v*.dat
existing_count=$(ls "$DEST_DIR"/"${DATE_STR}"_v*.dat 2>/dev/null | wc -l)
next_version=$(( existing_count + 1 ))

# Form backup filename
BACKUP_NAME="${DATE_STR}_v${next_version}_${SRC_FILE}"

# Copy the file
cp "$SRC_FILE" "$DEST_DIR/$BACKUP_NAME"

# Confirmation
echo "Backed up '$SRC_FILE' to '$DEST_DIR/$BACKUP_NAME'."


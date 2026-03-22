#!/usr/bin/env bash
# Bailout on first error.
set -e

# Get the script directory.
script_dir="$(cd "$(dirname "${0}")" && pwd)"

models_dir="${script_dir}/bin/config/models"
model="en_GB-vctk-medium"

if [ ! -f "${models_dir}/${model}.onnx" ]; then
	wget -P "${models_dir}/" "https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/vctk/medium/${model}.onnx"
fi

if [ ! -f "${models_dir}/${model}.onnx.json" ]; then
	wget -P "${models_dir}/" "https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/vctk/medium/${model}.onnx.json"
fi

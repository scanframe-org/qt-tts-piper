#!/usr/bin/env python3

import argparse
import json
import sys
import urllib.request
from pathlib import Path
from typing import List, Dict, Any, Optional, Tuple

# Global constants for URLs and paths.
VOICES_JSON_URL: str = "https://huggingface.co/rhasspy/piper-voices/resolve/main/voices.json"
BASE_DOWNLOAD_URL: str = "https://huggingface.co/rhasspy/piper-voices/resolve/main"
MODELS_DIR: Path = Path(__file__).parent / "bin" / "models"
# List of default voices to download.
DEFAULT_VOICES: List[List[str]] = [
	["en", "GB", "cori", "high"],
	["en", "US", "lessac", "medium"],
	["en", "US", "ljspeech", "medium"],
	["en", "US", "hfc_female", "medium"],
	["en" ,"US" ,"hfc_male", "medium"],
]

def download_file(url: str, destination: Path) -> None:
	"""Downloads a file from url to destination."""
	print(f"Downloading {url} to {destination}")
	try:
		urllib.request.urlretrieve(url, str(destination))
	except KeyboardInterrupt:
		raise
	except Exception as e:
		raise RuntimeError(f"Failed to download {url}") from e


def get_voices_json() -> Dict[str, Any]:
	"""Fetches and parses voices.json."""
	try:
		with urllib.request.urlopen(VOICES_JSON_URL) as response:
			return json.loads(response.read().decode())
	except Exception as e:
		raise RuntimeError(f"Failed to fetch {VOICES_JSON_URL}") from e


def interactive_mode(voices: Dict[str, Any], args: argparse.Namespace) -> Optional[Dict[str, str]]:
	"""Interactive mode to select a voice."""
	# Group voices by language family (e.g., 'en', 'nl').
	languages: List[str] = sorted(set(v['language']['family'] for v in voices.values()))
	lang_family: str = args.language
	if not lang_family:
		print("Available languages: " + ", ".join(languages))
		lang_family = input("Select language: ").strip()
	else:
		print(f"Selected language: {lang_family}")

	if lang_family not in languages:
		print(f"Invalid language: {lang_family}")
		return None

	# Filter by language family and group by region.
	lang_voices: List[Dict[str, Any]] = [v for v in voices.values() if v['language']['family'] == lang_family]
	regions: List[str] = sorted(set(v['language']['region'] for v in lang_voices))
	region: str = args.region
	if not region:
		print(f"Available regions for {lang_family}: " + ", ".join(regions))
		region = input("Select region: ").strip()
	else:
		print(f"Selected region: {region}")

	if region not in regions:
		print(f"Invalid region: {region}")
		return None

	# Filter by region and group by dataset/name.
	region_voices: List[Dict[str, Any]] = [v for v in lang_voices if v['language']['region'] == region]
	datasets: List[str] = sorted(set(v['name'] for v in region_voices))
	dataset: str = args.dataset
	if not dataset:
		print(f"Available datasets for {lang_family}_{region}: " + ", ".join(datasets))
		dataset = input("Select dataset: ").strip()
	else:
		print(f"Selected dataset: {dataset}")

	if dataset not in datasets:
		print(f"Invalid dataset: {dataset}")
		return None

	# Filter by dataset and group by quality.
	dataset_voices: List[Dict[str, Any]] = [v for v in region_voices if v['name'] == dataset]
	qualities: List[str] = sorted(set(v['quality'] for v in dataset_voices))
	quality: str = args.quality
	if not quality:
		print(f"Available qualities for {dataset}: " + ", ".join(qualities))
		quality = input("Select quality: ").strip()
	else:
		print(f"Selected quality: {quality}")

	if quality not in qualities:
		print(f"Invalid quality: {quality}")
		return None

	return {
		'language': lang_family,
		'region': region,
		'dataset': dataset,
		'quality': quality
	}


def perform_download(lang: str, region: str, dataset: str, quality: str, models_dir: Path, voices: Optional[Dict[str, Any]] = None) -> None:
	"""Performs the download of .onnx and .onnx.json files."""
	model_id: str = f"{lang}_{region}-{dataset}-{quality}"

	# Try to find the files from voices.json if provided.
	files_to_download: List[Tuple[str, str]] = []
	if voices:
		# Search for the voice in voices.json.
		voice_entry: Optional[Dict[str, Any]] = None
		# Try the exact key match first.
		if model_id in voices:
			voice_entry = voices[model_id]
		else:
			# Try to find by components.
			for v in voices.values():
				if (v['language']['family'] == lang and
					v['language']['region'] == region and
					v['name'] == dataset and
					v['quality'] == quality):
					voice_entry = v
					break

		if voice_entry and 'files' in voice_entry:
			# noinspection PyUnresolvedReferences
			for rel_path in voice_entry['files'].keys():
				if rel_path.endswith('.onnx') or rel_path.endswith('.onnx.json'):
					filename: str = rel_path.split('/')[-1]
					files_to_download.append((rel_path, filename))

	if not files_to_download:
		# Fallback to the default path structure if not found in 'voices.json' or when 'voices.json' is not provided.
		onnx_file: str = f"{model_id}.onnx"
		json_file: str = f"{onnx_file}.json"
		# Piper HuggingFace path structure: {lang}/{lang}_{region}/{dataset}/{quality}/{file}.
		download_base_path: str = f"{lang}/{lang}_{region}/{dataset}/{quality}"
		files_to_download = [
			(f"{download_base_path}/{onnx_file}", onnx_file),
			(f"{download_base_path}/{json_file}", json_file)
		]

	models_dir.mkdir(parents=True, exist_ok=True)

	for rel_path, filename in files_to_download:
		dest_path: Path = models_dir / filename
		if dest_path.exists():
			print(f"{filename} already exists. Skipping.")
			continue

		url: str = f"{BASE_DOWNLOAD_URL}/{rel_path}"
		download_file(url, dest_path)


def main() -> int:
	parser = argparse.ArgumentParser(description="Download Piper voice models.")
	parser.add_argument("-l", "--language", help="Language code (e.g., en, nl)")
	parser.add_argument("-r", "--region", help="Region code (e.g., GB, US, NL)")
	parser.add_argument("-d", "--dataset", help="Dataset name (e.g., vctk, lessac, pim)")
	parser.add_argument("-q", "--quality", help="Quality (e.g., low, medium, high)")
	parser.add_argument("-m", "--models-directory", help=f"Directory to store models (default: {MODELS_DIR})")
	parser.add_argument("-i", "--interactive", action="store_true", help="Interactive mode")
	parser.add_argument("--default", action="store_true", help="Download default voices")

	if len(sys.argv) == 1:
		parser.print_help()
		return 0

	args = parser.parse_args()
	models_dir = Path(args.models_directory) if args.models_directory else MODELS_DIR
	voices = None

	if args.default:
		for voice in DEFAULT_VOICES:
			language, region, dataset, quality = voice
			perform_download(language, region, dataset, quality, models_dir, voices)
		return 0

	if args.interactive or any([args.language, args.region, args.dataset, args.quality]):
		# We might need voices.json for either mode to be robust.
		# But let's only fetch it if needed or to be sure.
		voices = get_voices_json()

	if args.interactive:
		selection = interactive_mode(voices, args)
		if selection:
			perform_download(selection['language'], selection['region'], selection['dataset'], selection['quality'], models_dir, voices)
	else:
		if not all([args.language, args.region, args.dataset, args.quality]):
			print(
				"Error: All arguments (--language, --region, --dataset, --quality) are required unless using --interactive.")
			return 1

		perform_download(args.language, args.region, args.dataset, args.quality, models_dir, voices)

	return 0

if __name__ == "__main__":
	exitcode:int = 0
	try:
		exitcode = main()
	except KeyboardInterrupt:
		print("\n! Interrupted by user.")
		exitcode = 130
	except Exception as any_ex:
		print(f"! Exception({any_ex.__class__.__name__}): {any_ex}")
		if hasattr(any_ex, '__notes__'):
			for note in any_ex.__notes__:
				print(f"Note: {note}")
		exitcode = 1
	sys.exit(exitcode)
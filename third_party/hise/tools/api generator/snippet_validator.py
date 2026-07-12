"""
HISE API Documentation Example Validator

Reads examples directly from Phase 1/2/3 markdown source files and:
1. Reports coverage (examples, testMetadata, test results per phase)
2. Extracts examples for display
3. Adds/updates testMetadata in .md files
4. Edits example code in .md files
5. Validates examples via HISE REST API, records results in sidecar file

Source tags map to enrichment phases:
  auto    -> phase1/{Class}/methods.md
  project -> phase2/{Class}/{method}.md
  manual  -> phase3/{Class}/{method}.md
"""

import requests
import json
import re
import argparse
import os
import time
import subprocess
import platform
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

SOURCE_TO_PHASE = {
	"auto": "phase1",
	"project": "phase2",
	"manual": "phase3",
}

PHASE_TO_SOURCE = {v: k for k, v in SOURCE_TO_PHASE.items()}


# ---------------------------------------------------------------------------
# Colors
# ---------------------------------------------------------------------------

class Colors:
	"""ANSI color codes for terminal output"""
	GREEN = '\033[92m'
	RED = '\033[91m'
	YELLOW = '\033[93m'
	CYAN = '\033[96m'
	BOLD = '\033[1m'
	DIM = '\033[2m'
	RESET = '\033[0m'
	
	@staticmethod
	def green(text):
		return f"{Colors.GREEN}{text}{Colors.RESET}"
	
	@staticmethod
	def red(text):
		return f"{Colors.RED}{text}{Colors.RESET}"
	
	@staticmethod
	def yellow(text):
		return f"{Colors.YELLOW}{text}{Colors.RESET}"
	
	@staticmethod
	def cyan(text):
		return f"{Colors.CYAN}{text}{Colors.RESET}"
	
	@staticmethod
	def bold(text):
		return f"{Colors.BOLD}{text}{Colors.RESET}"
	
	@staticmethod
	def dim(text):
		return f"{Colors.DIM}{text}{Colors.RESET}"


# ---------------------------------------------------------------------------
# HISE REST API Client
# ---------------------------------------------------------------------------

class HISEConnectionError(Exception):
	"""Raised when the HISE REST API is unreachable (connection refused, timeout, etc.)."""
	pass


class HISEAPIClient:
	"""Thin wrapper around HISE REST API"""
	
	def __init__(self, base_url="http://127.0.0.1:1900", timeout=10):
		self.base_url = base_url
		self.timeout = timeout
		self.debug_timing = False
	
	def _request(self, method: str, path: str, **kwargs):
		"""Send an HTTP request, converting connection failures to HISEConnectionError."""
		url = f"{self.base_url}{path}"
		kwargs.setdefault("timeout", self.timeout)
		try:
			t0 = time.perf_counter()
			response = getattr(requests, method)(url, **kwargs)
			elapsed = time.perf_counter() - t0
			if self.debug_timing:
				print(f"  [{method.upper()} {path} -> {elapsed:.3f}s]")
			return response.json()
		except requests.exceptions.ConnectionError:
			raise HISEConnectionError(f"Connection refused - is HISE running? ({url})")
		except requests.exceptions.Timeout:
			raise HISEConnectionError(f"Request timed out ({url})")
		except requests.exceptions.RequestException as e:
			raise HISEConnectionError(f"Request failed: {e}")
	
	def status(self):
		"""GET /api/status"""
		return self._request("get", "/api/status")
	
	def set_script(self, module_id: str, callbacks: Dict[str, str], compile=True):
		"""POST /api/set_script"""
		return self._request("post", "/api/set_script", json={
			"moduleId": module_id,
			"callbacks": callbacks,
			"compile": compile
		})
	
	def repl(self, module_id: str, expression: str):
		"""POST /api/repl"""
		return self._request("post", "/api/repl", json={
			"moduleId": module_id,
			"expression": expression
		})

	def builder_reset(self):
		"""POST /api/builder/reset — Reset module tree to empty state."""
		return self._request("post", "/api/builder/reset")

	def builder_apply(self, operations: list):
		"""POST /api/builder/apply — Add/remove/configure modules in batch."""
		return self._request("post", "/api/builder/apply", json={
			"operations": operations
		})

	def ui_apply(self, module_id: str, operations: list):
		"""POST /api/ui/apply — Add/remove/configure UI components in batch."""
		return self._request("post", "/api/ui/apply", json={
			"moduleId": module_id,
			"operations": operations
		})

	def set_component_value(self, module_id: str, component_id: str, value):
		"""POST /api/set_component_value — Set value and trigger callback."""
		return self._request("post", "/api/set_component_value", json={
			"moduleId": module_id,
			"id": component_id,
			"value": value
		})


# ---------------------------------------------------------------------------
# HISE Launcher
# ---------------------------------------------------------------------------

class HISELauncher:
	"""Manages HISE process lifecycle for automated validation."""
	
	def __init__(self, debug=True, port=1900):
		self.port = port
		self.api_url = f"http://127.0.0.1:{port}"
		self.process = None
		self.we_launched = False
		
		# Resolve executable name per platform
		name = "HISE Debug" if debug else "HISE"
		if platform.system() == "Windows":
			self.exe_name = f"{name}.exe"
		else:
			self.exe_name = name
	
	def is_running(self) -> bool:
		"""Check if HISE REST API is reachable."""
		try:
			r = requests.get(f"{self.api_url}/api/status", timeout=2)
			return r.status_code == 200
		except requests.exceptions.RequestException:
			return False
	
	def launch(self, timeout=30) -> bool:
		"""Launch HISE with start_server, poll until ready."""
		if self.is_running():
			print(f"[OK] HISE already running on port {self.port}")
			self.we_launched = False
			return True
		
		cmd = [self.exe_name, "start_server", f"-port:{self.port}"]
		print(f"Launching {self.exe_name} on port {self.port}...")
		
		try:
			self.process = subprocess.Popen(
				cmd,
				stdout=subprocess.DEVNULL,
				stderr=subprocess.DEVNULL
			)
		except FileNotFoundError:
			print(Colors.red(f"[ERROR] '{self.exe_name}' not found on PATH"))
			return False
		
		self.we_launched = True
		
		# Poll for readiness
		start = time.perf_counter()
		while time.perf_counter() - start < timeout:
			time.sleep(1)
			if self.is_running():
				elapsed = time.perf_counter() - start
				print(f"[OK] HISE ready ({elapsed:.1f}s)")
				return True
			# Check if process died
			if self.process.poll() is not None:
				print(Colors.red(f"[ERROR] HISE exited with code {self.process.returncode}"))
				return False
		
		print(Colors.red(f"[ERROR] HISE failed to start within {timeout}s"))
		self.process.terminate()
		return False
	
	def shutdown(self) -> bool:
		"""Send POST /api/shutdown, wait for process to exit."""
		try:
			r = requests.post(f"{self.api_url}/api/shutdown", timeout=5)
			if r.status_code == 200:
				print("Shutdown request sent")
		except requests.exceptions.RequestException:
			print(Colors.red("[WARNING] Could not send shutdown request"))
			return False
		
		if self.process:
			try:
				self.process.wait(timeout=10)
				print("[OK] HISE shut down")
			except subprocess.TimeoutExpired:
				print(Colors.yellow("[WARNING] HISE didn't exit, terminating"))
				self.process.terminate()
		else:
			# No process handle - wait for port to become unavailable
			for _ in range(10):
				time.sleep(1)
				if not self.is_running():
					print("[OK] HISE shut down")
					return True
		
		return True
	
	def cleanup(self, keep_alive=False):
		"""Called after validation. Shuts down only if we launched."""
		if self.we_launched and not keep_alive:
			self.shutdown()


# ---------------------------------------------------------------------------
# Markdown Example Reader
# ---------------------------------------------------------------------------

class MarkdownExampleReader:
	"""Reads examples from Phase 1/2/3 markdown source files.
	
	Each phase has its own directory layout:
	  phase1/{Class}/methods.md        - all methods in one file, ## headings
	  phase2/{Class}/{method}.md       - one file per method
	  phase3/{Class}/{method}.md       - one file per method, PascalCase dirs
	
	Directory lookups are case-insensitive for all phases.
	"""
	
	def __init__(self, enrichment_dir: Optional[Path] = None):
		if enrichment_dir is None:
			enrichment_dir = Path(__file__).parent / "enrichment"
		self.enrichment_dir = enrichment_dir
		
		# Build case-insensitive directory indexes on init
		self._dir_index = {}  # phase -> {lowercase_name: actual_dir_name}
		for source, phase in SOURCE_TO_PHASE.items():
			phase_dir = self.enrichment_dir / phase
			if phase_dir.is_dir():
				self._dir_index[source] = {
					d.name.lower(): d.name
					for d in phase_dir.iterdir()
					if d.is_dir()
				}
			else:
				self._dir_index[source] = {}
	
	def _resolve_class_dir(self, source: str, class_name: str) -> Optional[Path]:
		"""Resolve class directory with case-insensitive lookup."""
		index = self._dir_index.get(source, {})
		actual_name = index.get(class_name.lower())
		if actual_name is None:
			return None
		phase = SOURCE_TO_PHASE[source]
		return self.enrichment_dir / phase / actual_name
	
	def _resolve_method_file(self, source: str, class_name: str, method_name: str) -> Optional[Path]:
		"""Resolve the .md file path for a given source/class/method."""
		class_dir = self._resolve_class_dir(source, class_name)
		if class_dir is None:
			return None
		
		if source == "auto":
			# Phase 1: single methods.md file
			path = class_dir / "methods.md"
			return path if path.is_file() else None
		else:
			# Phase 2/3: one .md per method, case-insensitive filename lookup
			for md_file in class_dir.glob("*.md"):
				if md_file.name.lower() == "readme.md":
					continue
				if md_file.stem.lower() == method_name.lower():
					return md_file
			return None
	
	def resolve_path(self, source: str, class_name: str, method_name: Optional[str] = None) -> Optional[Path]:
		"""Public path resolver for edits/metadata writes."""
		if method_name:
			return self._resolve_method_file(source, class_name, method_name)
		class_dir = self._resolve_class_dir(source, class_name)
		if class_dir and source == "auto":
			path = class_dir / "methods.md"
			return path if path.is_file() else None
		return class_dir
	
	def list_classes(self, source: str) -> List[str]:
		"""List class names that have a directory in the given phase."""
		index = self._dir_index.get(source, {})
		return sorted(index.values())
	
	def list_methods(self, source: str, class_name: str) -> List[str]:
		"""List method names that have examples in the given phase/class."""
		class_dir = self._resolve_class_dir(source, class_name)
		if class_dir is None:
			return []
		
		if source == "auto":
			# Phase 1: parse ## headings from methods.md
			methods_file = class_dir / "methods.md"
			if not methods_file.is_file():
				return []
			content = methods_file.read_text(encoding="utf-8")
			# Find ## headings and check if they have code blocks
			methods = []
			sections = re.split(r"^## (\w+)\s*$", content, flags=re.MULTILINE)
			# sections: [preamble, name1, body1, name2, body2, ...]
			for i in range(1, len(sections), 2):
				method_name = sections[i]
				body = sections[i + 1] if i + 1 < len(sections) else ""
				if re.search(r"```(?:javascript|js)", body):
					methods.append(method_name)
			return sorted(methods)
		else:
			# Phase 2/3: each .md file (except Readme) is a method
			methods = []
			for md_file in class_dir.glob("*.md"):
				if md_file.name.lower() == "readme.md":
					continue
				# Check if it has code blocks
				content = md_file.read_text(encoding="utf-8")
				if re.search(r"```(?:javascript|js)", content):
					methods.append(md_file.stem)
			return sorted(methods)
	
	def get_examples(self, source: str, class_name: str, method_name: str) -> List[Dict]:
		"""Parse and return all examples for a given source/class/method.
		
		Returns list of dicts with keys:
		  slug, title, code, testMetadata (optional), source, filePath
		"""
		if source == "auto":
			return self._get_phase1_examples(class_name, method_name)
		else:
			return self._get_phase2or3_examples(source, class_name, method_name)
	
	def get_example_by_slug(self, source: str, class_name: str,
	                        method_name: str, slug: str) -> Optional[Dict]:
		"""Get a single example by slug."""
		examples = self.get_examples(source, class_name, method_name)
		for ex in examples:
			if ex.get("slug") == slug:
				return ex
		return None
	
	def _get_phase1_examples(self, class_name: str, method_name: str) -> List[Dict]:
		"""Parse examples from phase1/{Class}/methods.md for a specific method."""
		md_path = self._resolve_method_file("auto", class_name, method_name)
		if md_path is None:
			return []
		
		content = md_path.read_text(encoding="utf-8")
		
		# Extract the section for this specific method
		sections = re.split(r"^## (\w+)\s*$", content, flags=re.MULTILINE)
		method_body = None
		for i in range(1, len(sections), 2):
			if sections[i] == method_name:
				method_body = sections[i + 1] if i + 1 < len(sections) else ""
				break
		
		if method_body is None:
			return []
		
		return self._parse_examples_from_text(method_body, "auto", str(md_path))
	
	def _get_phase2or3_examples(self, source: str, class_name: str,
	                            method_name: str) -> List[Dict]:
		"""Parse examples from phase2 or phase3 method file."""
		md_path = self._resolve_method_file(source, class_name, method_name)
		if md_path is None:
			return []
		
		content = md_path.read_text(encoding="utf-8")
		return self._parse_examples_from_text(content, source, str(md_path))
	
	def _parse_examples_from_text(self, text: str, source: str,
	                              file_path: str) -> List[Dict]:
		"""Parse example code blocks from markdown text.
		
		Supports:
		1. New format: ```javascript:slug with optional testMetadata blocks
		2. Legacy format: ```javascript (no slug) - auto-generates slugs
		"""
		examples = []
		
		# Try new format first (slugged blocks)
		slug_blocks = re.findall(
			r"```javascript:(\S+)\s*\n(.*?)```",
			text, re.DOTALL
		)
		
		if slug_blocks:
			for slug, code_raw in slug_blocks:
				code_raw = code_raw.strip()
				
				# Extract // Title: comment (first line only)
				title = ""
				title_match = re.match(r"//\s*Title:\s*(.+)", code_raw)
				if title_match:
					title = title_match.group(1).strip()
					code_raw = re.sub(r"^//\s*Title:.*\n", "", code_raw, count=1)
				
				# Extract setup block
				setup_script = ""
				setup_match = re.search(
					r"//\s*---\s*setup\s*---\s*\n(.*?)//\s*---\s*end setup\s*---\s*\n",
					code_raw, re.DOTALL | re.IGNORECASE
				)
				if setup_match:
					setup_script = setup_match.group(1).strip()
					code_raw = re.sub(
						r"//\s*---\s*setup\s*---\s*\n.*?//\s*---\s*end setup\s*---\s*\n",
						"", code_raw, flags=re.DOTALL | re.IGNORECASE
					)
				
				# Extract test-only block
				test_only_code = ""
				test_only_match = re.search(
					r"//\s*---\s*test-only\s*---\s*\n(.*?)//\s*---\s*end test-only\s*---\s*\n?",
					code_raw, re.DOTALL | re.IGNORECASE
				)
				if test_only_match:
					test_only_code = test_only_match.group(1).strip()
					code_raw = re.sub(
						r"//\s*---\s*test-only\s*---\s*\n.*?//\s*---\s*end test-only\s*---\s*\n?",
						"", code_raw, flags=re.DOTALL | re.IGNORECASE
					)
				
				code = code_raw.strip()
				
				# Look for matching testMetadata block
				metadata_pattern = r"```json:testMetadata:" + re.escape(slug) + r"\s*\n(.*?)```"
				metadata_match = re.search(metadata_pattern, text, re.DOTALL)
				
				example = {
					"slug": slug,
					"title": title or "Example",
					"code": code,
					"source": source,
					"filePath": file_path,
				}
				
				if metadata_match:
					try:
						metadata = json.loads(metadata_match.group(1))
						if setup_script:
							metadata["setupScript"] = setup_script
						if test_only_code:
							metadata["testOnly"] = test_only_code
						example["testMetadata"] = metadata
					except json.JSONDecodeError:
						pass
				elif setup_script or test_only_code:
					metadata = {}
					if setup_script:
						metadata["setupScript"] = setup_script
					if test_only_code:
						metadata["testOnly"] = test_only_code
					example["testMetadata"] = metadata
				
				examples.append(example)
		else:
			# Legacy format - auto-generate slugs from filename
			basename = Path(file_path).stem  # e.g. "sample", "methods"
			
			blocks = re.findall(
				r"(?:(?://\s*(.+)\n)|(?:###\s*(.+)\n))?```(?:javascript|js)\s*\n(.*?)```",
				text, re.DOTALL
			)
			
			for idx, (title_comment, title_heading, code) in enumerate(blocks):
				title = (title_heading or title_comment or "").strip()
				slug = f"{basename}-{idx + 1}"
				
				examples.append({
					"slug": slug,
					"title": title or "Example",
					"code": code.strip(),
					"source": source,
					"filePath": file_path,
				})
		
		return examples


# ---------------------------------------------------------------------------
# Sidecar Result Store
# ---------------------------------------------------------------------------

class SidecarResultStore:
	"""Manages enrichment/output/test_results.json.
	
	Flat key-value store. Keys: "Class.method.source.slug"
	Each --validate run accumulates results; only tested entries are overwritten.
	"""
	
	def __init__(self, output_dir: Optional[Path] = None):
		if output_dir is None:
			output_dir = Path(__file__).parent / "enrichment" / "output"
		self.path = output_dir / "test_results.json"
		self._data = None
	
	def load(self) -> Dict:
		"""Load from disk, or return empty structure."""
		if self._data is not None:
			return self._data
		
		if self.path.is_file():
			with open(self.path, "r", encoding="utf-8") as f:
				self._data = json.load(f)
		else:
			self._data = {"lastUpdated": None, "results": {}}
		
		return self._data
	
	def save(self):
		"""Write to disk."""
		data = self.load()
		data["lastUpdated"] = datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
		self.path.parent.mkdir(parents=True, exist_ok=True)
		with open(self.path, "w", encoding="utf-8") as f:
			json.dump(data, f, indent=2, ensure_ascii=False)
	
	def make_key(self, class_name: str, method_name: str, source: str, slug: str) -> str:
		"""Build sidecar key: Class.method.source.slug"""
		return f"{class_name}.{method_name}.{source}.{slug}"
	
	def record(self, key: str, result: Dict):
		"""Upsert a single test result."""
		data = self.load()
		data["results"][key] = result
	
	def get(self, key: str) -> Optional[Dict]:
		"""Retrieve a single test result, or None."""
		data = self.load()
		return data["results"].get(key)
	
	def get_all(self) -> Dict:
		"""Return all results."""
		return self.load().get("results", {})
	
	def get_for_method(self, class_name: str, method_name: str) -> Dict[str, Dict]:
		"""Get all results for a class.method (any source/slug).
		
		Returns: {key: result_dict, ...}
		"""
		prefix = f"{class_name}.{method_name}."
		results = self.load().get("results", {})
		return {k: v for k, v in results.items() if k.startswith(prefix)}


# ---------------------------------------------------------------------------
# Snippet Validator (test executor)
# ---------------------------------------------------------------------------

class SnippetValidator:
	"""3-step test executor: setup -> execute -> verify via HISE REST API"""
	
	def __init__(self, api_client: HISEAPIClient, module_id="Interface"):
		self.api = api_client
		self.module_id = module_id
	
	def clear_script(self):
		"""Clear script to reset state"""
		self.api.set_script(self.module_id, {"onInit": ""})
	
	_TESTCALLBACK_PREFIXES = (
		"warning: this should be only used",
		"BEGIN_CALLBACK_TEST",
		"END_CALLBACK_TEST",
		"CALLBACK_ARGS:",
	)
	
	def _filter_test_noise(self, logs):
		"""Strip Console.testCallback diagnostic markers from log output"""
		return [l for l in logs if not any(l.strip().startswith(p) for p in self._TESTCALLBACK_PREFIXES)]
	
	def _format_value(self, val) -> str:
		"""Format a value for display"""
		if val == "undefined":
			return "undefined"
		if isinstance(val, str):
			return f'"{val}"'
		if isinstance(val, bool):
			return "true" if val else "false"
		if isinstance(val, (int, float)):
			return str(val)
		return json.dumps(val)
	
	def _normalize_error_message(self, error_obj) -> str:
		"""Strip line/column info from error message for matching"""
		if isinstance(error_obj, dict):
			msg = error_obj.get("errorMessage", "")
		elif isinstance(error_obj, str):
			msg = error_obj
		else:
			return ""
		msg = re.sub(r'^Line \d+, column \d+:\s*', '', msg)
		return msg
	
	def _values_match(self, expected, actual) -> bool:
		"""Compare REPL values with type normalization"""
		if actual == "undefined":
			return expected is None or expected == "undefined"
		if expected == actual:
			return True
		try:
			return float(expected) == float(actual)
		except (ValueError, TypeError):
			pass
		return str(expected).strip() == str(actual).strip()
	
	def _logs_match(self, expected: list, actual: list) -> bool:
		"""Smart log comparison with type normalization and prefix stripping"""
		if len(expected) != len(actual):
			return False
		
		for exp, act in zip(expected, actual):
			exp_str = str(exp).strip()
			act_str = str(act).strip()
			
			for prefix in ["Interface: ", "Script Processor: ", "ScriptProcessor: "]:
				if act_str.startswith(prefix):
					act_str = act_str[len(prefix):]
			
			if exp_str == act_str:
				continue
			
			try:
				if float(exp_str) == float(act_str):
					continue
			except (ValueError, TypeError):
				pass
			
			if self._try_json_match(exp_str, act_str):
				continue
			
			return False
		
		return True
	
	def _try_json_match(self, exp_str: str, act_str: str) -> bool:
		"""Try JSON structural comparison, fall back to whitespace normalization."""
		try:
			exp_json = json.loads(exp_str)
			act_json = json.loads(act_str)
			return exp_json == act_json
		except (json.JSONDecodeError, TypeError, ValueError):
			pass
		
		exp_normalized = ' '.join(exp_str.split())
		act_normalized = ' '.join(act_str.split())
		return exp_normalized == act_normalized
	
	def _log_diff(self, expected: list, actual: list) -> str:
		"""Generate colored diff string for log mismatch"""
		lines = []
		lines.append(f"Expected {Colors.bold(str(len(expected)))} log entries, got {Colors.bold(str(len(actual)))}")
		lines.append("")
		
		max_len = max(len(expected), len(actual))
		for i in range(max_len):
			if i < len(expected) and i < len(actual):
				exp_val = expected[i]
				act_val = actual[i]
				match = self._logs_match([exp_val], [act_val])
				
				if match:
					lines.append(f"  [{i}] {Colors.green('[OK]')} {act_val}")
				else:
					lines.append(f"  [{i}] {Colors.red('[X]')} Expected: {Colors.cyan(str(exp_val))}")
					lines.append(f"       Got:      {Colors.yellow(str(act_val))}")
			elif i >= len(expected):
				lines.append(f"  [{i}] {Colors.red('[X]')} Unexpected: {Colors.yellow(str(actual[i]))}")
			else:
				lines.append(f"  [{i}] {Colors.red('[X]')} Missing: {Colors.cyan(str(expected[i]))}")
		
		return "\n".join(lines)
	
	def test_example(self, example: Dict) -> Dict:
		"""Execute 3-step test, return result dict for sidecar storage."""
		metadata = example.get("testMetadata", {})
		
		if not metadata.get("testable", False):
			return {
				"tested": False,
				"skipped": True,
				"reason": metadata.get("skipReason", "Not marked as testable"),
				"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
			}
		
		code = example.get("code", "")
		if not code:
			return {
				"tested": False,
				"skipped": True,
				"reason": "Missing code field",
				"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
			}
		
		setup_script = metadata.get("setupScript", "")
		test_only_code = metadata.get("testOnly", "")
		verify_scripts = metadata.get("verifyScript")
		
		# Step 1: Setup (always runs to ensure clean state)
		if setup_script:
			result = self.api.set_script(self.module_id, {"onInit": setup_script})
			if not result.get("success") or result.get("errors"):
				return {
					"tested": True,
					"passed": False,
					"stage": "setup",
					"error": result.get("errors", []),
					"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
				}
			time.sleep(0.2)
		else:
			self.clear_script()
		
		# Step 2: Execute the actual example code (with test-only code appended)
		full_code = code + "\n" + test_only_code if test_only_code else code
		result = self.api.set_script(self.module_id, {"onInit": full_code})
		
		# Check if we're expecting an error
		expecting_error = False
		if verify_scripts:
			scripts_list = verify_scripts if isinstance(verify_scripts, list) else [verify_scripts]
			for vs in scripts_list:
				if vs.get("type") == "expect-error":
					expecting_error = True
					break
		
		if not expecting_error:
			if not result.get("success") or result.get("errors"):
				return {
					"tested": True,
					"passed": False,
					"stage": "execute",
					"error": result.get("errors", []),
					"logs": self._filter_test_noise(result.get("logs", [])),
					"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
				}
		
		time.sleep(0.2)
		
		# Step 3: Verify (if provided)
		verifications = []
		
		if verify_scripts:
			if isinstance(verify_scripts, dict):
				verify_scripts = [verify_scripts]
			
			for i, verify_script in enumerate(verify_scripts):
				delay_ms = verify_script.get("delay", 100 if i > 0 else 0)
				delay_ms = min(delay_ms, 1000)
				if delay_ms > 0:
					time.sleep(delay_ms / 1000.0)
				verify_type = verify_script.get("type")
				
				if verify_type == "log-output":
					expected = verify_script.get("values", [])
					actual = self._filter_test_noise(result.get("logs", []))
					
					verifications.append({
						"type": "log-output",
						"delay": delay_ms,
						"expected": expected,
						"actual": list(actual),
					})
					
					if not self._logs_match(expected, actual):
						diff = self._log_diff(expected, actual)
						return {
							"tested": True,
							"passed": False,
							"stage": "verify",
							"error": f"Log output mismatch:\n{diff}",
							"verifications": verifications,
							"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
						}
				
				elif verify_type == "REPL":
					expression = verify_script.get("expression", "")
					expected_value = verify_script.get("value")
					
					if expected_value is None:
						return {
							"tested": False,
							"skipped": True,
							"reason": "REPL verification missing required 'value' field",
							"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
						}
					
					verify_result = self.api.repl(self.module_id, expression)
					
					if not verify_result.get("success") or verify_result.get("errors"):
						verifications.append({
							"type": "REPL",
							"delay": delay_ms,
							"expression": expression,
							"expected": expected_value,
							"actual": None,
							"error": verify_result.get("errors", []),
						})
						return {
							"tested": True,
							"passed": False,
							"stage": "verify",
							"error": verify_result.get("errors", []),
							"verifications": verifications,
							"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
						}
					
					actual_value = verify_result.get("value")
					verifications.append({
						"type": "REPL",
						"delay": delay_ms,
						"expression": expression,
						"expected": expected_value,
						"actual": actual_value,
					})
					
					if not self._values_match(expected_value, actual_value):
						exp_str = self._format_value(expected_value)
						act_str = self._format_value(actual_value)
						return {
							"tested": True,
							"passed": False,
							"stage": "verify",
							"error": f"Expected {Colors.cyan(expression)} -> {Colors.green(exp_str)}, got {Colors.red(act_str)}",
							"verifications": verifications,
							"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
						}
				
				elif verify_type == "expect-error":
					expected_pattern = verify_script.get("errorMessage", "")
					execution_failed = not result.get("success") or bool(result.get("errors"))
					
					if not execution_failed:
						verifications.append({
							"type": "expect-error",
							"delay": delay_ms,
							"expectedPattern": expected_pattern,
							"actualError": None,
							"error": "Expected error but execution succeeded",
						})
						return {
							"tested": True,
							"passed": False,
							"stage": "verify",
							"error": "Expected error but execution succeeded",
							"verifications": verifications,
							"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
						}
					
					errors = result.get("errors", [])
					if not errors:
						verifications.append({
							"type": "expect-error",
							"delay": delay_ms,
							"expectedPattern": expected_pattern,
							"actualError": None,
							"error": "Expected error but no error message in response",
						})
						return {
							"tested": True,
							"passed": False,
							"stage": "verify",
							"error": "Expected error but no error message in response",
							"verifications": verifications,
							"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
						}
					
					actual_msg = self._normalize_error_message(errors[0])
					verifications.append({
						"type": "expect-error",
						"delay": delay_ms,
						"expectedPattern": expected_pattern,
						"actualError": actual_msg,
					})
					
					if expected_pattern.lower() not in actual_msg.lower():
						return {
							"tested": True,
							"passed": False,
							"stage": "verify",
							"error": f"Error message mismatch:\n  Expected pattern: {expected_pattern}\n  Actual error: {actual_msg}",
							"verifications": verifications,
							"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
						}
		
		return {
			"tested": True,
			"passed": True,
			"verifications": verifications,
			"timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
		}


# ---------------------------------------------------------------------------
# Display helpers
# ---------------------------------------------------------------------------

def format_test_metadata(metadata):
	"""Format test metadata for human-readable display."""
	if not metadata:
		return "Test Metadata: None"
	
	lines = ["Test Metadata:"]
	
	testable = metadata.get("testable")
	if testable is None:
		lines.append("  Testable: (not specified)")
	else:
		lines.append(f"  Testable: {str(testable).lower()}")
	
	if not testable:
		return "\n".join(lines)
	
	setup = metadata.get("setupScript", "")
	if setup == "":
		lines.append("  Setup: (empty)")
	else:
		lines.append("  Setup:")
		for setup_line in setup.split("\n"):
			lines.append(f"    {setup_line}")
	
	verify = metadata.get("verifyScript")
	if not verify:
		lines.append("  Verification: (missing)")
		return "\n".join(lines)
	
	if isinstance(verify, list):
		check_count = len(verify)
		lines.append(f"  Verification: Mixed ({check_count} checks)")
		
		for idx, check in enumerate(verify):
			check_type = check.get("type", "REPL")
			
			if check_type == "log-output":
				values = check.get("values", [])
				lines.append(f"    [{idx}] log-output ({len(values)} values)")
				for v_idx, val in enumerate(values):
					lines.append(f"      - [{v_idx}] {val}")
			elif check_type == "expect-error":
				error_pattern = check.get("errorMessage", "")
				lines.append(f"    [{idx}] expect-error")
				lines.append(f"        Pattern: {error_pattern}")
			else:
				expr = check.get("expression", "(no expression)")
				val = check.get("value", "(no value)")
				lines.append(f"    [{idx}] {expr} -> {val}")
	else:
		verify_type = verify.get("type", "REPL")
		
		if verify_type == "log-output":
			values = verify.get("values", [])
			lines.append(f"  Verification: log-output ({len(values)} values)")
			for idx, val in enumerate(values):
				lines.append(f"    - [{idx}] {val}")
		elif verify_type == "expect-error":
			error_pattern = verify.get("errorMessage", "")
			lines.append(f"  Verification: expect-error")
			lines.append(f"    Pattern: {error_pattern}")
		else:
			expr = verify.get("expression", "(no expression)")
			val = verify.get("value", "(no value)")
			lines.append(f"  Verification: REPL (1 check)")
			lines.append(f"    - {expr} -> {val}")
	
	return "\n".join(lines)


def format_result_badge(result: Optional[Dict]) -> str:
	"""Format a sidecar result as a short badge for coverage output."""
	if result is None:
		return Colors.dim("[untested]")
	if result.get("skipped"):
		return Colors.yellow("[skip]")
	if result.get("passed"):
		return Colors.green("[pass]")
	return Colors.red("[FAIL]")


# ---------------------------------------------------------------------------
# Markdown file editing
# ---------------------------------------------------------------------------

def update_code_in_markdown(md_path: Path, slug: str, new_code: str) -> Tuple[bool, str]:
	"""Update example code in a markdown file by slug.
	
	Preserves the // Title: line if present.
	Returns: (success, message)
	"""
	content = md_path.read_text(encoding="utf-8")
	
	pattern = rf"(```javascript:{re.escape(slug)}\s*\n)(.*?)(```)"
	match = re.search(pattern, content, re.DOTALL)
	
	if not match:
		return (False, f"Code block with slug '{slug}' not found in {md_path}")
	
	fence_start, old_code, fence_end = match.groups()
	
	# Preserve title line
	old_lines = old_code.strip().split('\n')
	title_line = ""
	if old_lines and old_lines[0].strip().startswith("// Title:"):
		title_line = old_lines[0] + "\n"
	
	new_block = fence_start
	if title_line:
		new_block += title_line
	new_block += new_code.strip() + "\n"
	new_block += fence_end
	
	new_content = content[:match.start()] + new_block + content[match.end():]
	md_path.write_text(new_content, encoding="utf-8")
	return (True, f"Updated code in {md_path}")


def update_metadata_in_markdown(md_path: Path, slug: str,
                                test_metadata: dict) -> Tuple[bool, str]:
	"""Update or insert testMetadata block in a markdown file by slug.
	
	Returns: (success, message)
	"""
	content = md_path.read_text(encoding="utf-8")
	
	code_pattern = rf"(```javascript:{re.escape(slug)}\s*\n.*?```)"
	code_match = re.search(code_pattern, content, re.DOTALL)
	
	if not code_match:
		return (False, f"Code block with slug '{slug}' not found in {md_path}")
	
	code_block_end = code_match.end()
	
	# Format new metadata block
	metadata_json = json.dumps(test_metadata, indent=2)
	new_metadata_block = f"\n```json:testMetadata:{slug}\n{metadata_json}\n```\n"
	
	# Check for existing metadata block after the code block
	metadata_pattern = rf"\s*```json:testMetadata:{re.escape(slug)}\s*\n.*?```"
	after_code = content[code_block_end:]
	metadata_match = re.match(metadata_pattern, after_code, re.DOTALL)
	
	if metadata_match:
		replacement_end = code_block_end + metadata_match.end()
		new_content = content[:code_block_end] + new_metadata_block + content[replacement_end:]
	else:
		new_content = content[:code_block_end] + new_metadata_block + content[code_block_end:]
	
	md_path.write_text(new_content, encoding="utf-8")
	return (True, f"Updated testMetadata in {md_path}")


# ---------------------------------------------------------------------------
# CLI Mode Handlers
# ---------------------------------------------------------------------------

def cmd_coverage(args, reader: MarkdownExampleReader, sidecar: SidecarResultStore):
	"""Generate coverage report."""
	sources = ["auto", "project", "manual"] if args.source == "all" else [args.source]
	
	if args.all_classes:
		return _coverage_all_classes(sources, reader, sidecar)
	else:
		return _coverage_single_class(args.class_name, sources, reader, sidecar)


def _coverage_all_classes(sources: List[str], reader: MarkdownExampleReader,
                          sidecar: SidecarResultStore) -> int:
	"""Compact coverage for all classes across specified sources."""
	sidecar.load()
	
	# Collect all class names across all requested sources
	all_classes = set()
	for source in sources:
		all_classes.update(reader.list_classes(source))
	
	if not all_classes:
		print("No classes found in the specified source(s).")
		return 1
	
	print(f"=== Coverage Report: {', '.join(sources)} ===\n")
	
	grand_examples = 0
	grand_testable = 0
	grand_tested = 0
	grand_passed = 0
	
	for class_name in sorted(all_classes):
		class_examples = 0
		class_testable = 0
		class_tested = 0
		class_passed = 0
		source_details = []
		
		for source in sources:
			methods = reader.list_methods(source, class_name)
			if not methods:
				continue
			
			src_examples = 0
			src_testable = 0
			src_tested = 0
			src_passed = 0
			
			for method_name in methods:
				examples = reader.get_examples(source, class_name, method_name)
				for ex in examples:
					src_examples += 1
					meta = ex.get("testMetadata", {})
					if meta.get("testable") is not None:
						src_testable += 1
					
					key = sidecar.make_key(class_name, method_name, source, ex["slug"])
					result = sidecar.get(key)
					if result and result.get("tested"):
						src_tested += 1
						if result.get("passed"):
							src_passed += 1
			
			if src_examples > 0:
				source_details.append((source, src_examples, src_testable, src_tested, src_passed))
				class_examples += src_examples
				class_testable += src_testable
				class_tested += src_tested
				class_passed += src_passed
		
		grand_examples += class_examples
		grand_testable += class_testable
		grand_tested += class_tested
		grand_passed += class_passed
		
		if source_details:
			parts = []
			for src, ex_count, testable_count, tested_count, passed_count in source_details:
				part = f"{src}: {ex_count} examples"
				if tested_count > 0:
					part += f", {passed_count}/{tested_count} passed"
				elif testable_count > 0:
					part += f", {testable_count} testable"
				parts.append(part)
			
			print(f"{Colors.bold(class_name)}: {' | '.join(parts)}")
		else:
			print(f"{Colors.dim(class_name)}: no examples")
	
	print(f"\n=== Summary ===")
	print(f"Classes: {len(all_classes)}")
	print(f"Examples: {grand_examples}")
	if grand_tested > 0:
		print(f"Tested: {grand_tested}, Passed: {grand_passed} ({grand_passed/max(grand_tested,1)*100:.1f}%)")
	elif grand_testable > 0:
		print(f"Testable: {grand_testable} (none tested yet)")
	
	return 0


def _coverage_single_class(class_name: str, sources: List[str],
                           reader: MarkdownExampleReader,
                           sidecar: SidecarResultStore) -> int:
	"""Detailed coverage for one class."""
	sidecar.load()
	
	print(f"=== {class_name} Coverage ===\n")
	
	found_any = False
	
	for source in sources:
		methods = reader.list_methods(source, class_name)
		if not methods:
			continue
		
		found_any = True
		phase = SOURCE_TO_PHASE[source]
		print(f"--- {source} ({phase}) ---")
		
		total_examples = 0
		testable_count = 0
		tested_count = 0
		passed_count = 0
		
		for method_name in sorted(methods):
			examples = reader.get_examples(source, class_name, method_name)
			if not examples:
				continue
			
			badges = []
			for ex in examples:
				total_examples += 1
				meta = ex.get("testMetadata", {})
				
				key = sidecar.make_key(class_name, method_name, source, ex["slug"])
				result = sidecar.get(key)
				
				is_testable = meta.get("testable") is not None
				if is_testable:
					testable_count += 1
				
				if result and result.get("tested"):
					tested_count += 1
					if result.get("passed"):
						passed_count += 1
				
				badge = format_result_badge(result)
				slug_display = Colors.dim(f":{ex['slug']}")
				badges.append(f"{badge} {slug_display}")
			
			print(f"  {method_name}: {', '.join(badges)}")
		
		print(f"  Total: {total_examples} examples, {testable_count} testable", end="")
		if tested_count > 0:
			print(f", {passed_count}/{tested_count} passed")
		else:
			print()
		print()
	
	if not found_any:
		# Check if the class exists in any phase
		all_classes = set()
		for s in ["auto", "project", "manual"]:
			all_classes.update(reader.list_classes(s))
		
		if class_name not in all_classes:
			print(f"Class '{class_name}' not found in any phase.")
			close = [c for c in sorted(all_classes) if class_name.lower() in c.lower()]
			if close:
				print(f"Did you mean: {', '.join(close[:5])}")
		else:
			print(f"No examples found for {class_name} in source(s): {', '.join(sources)}")
		return 1
	
	return 0


def cmd_extract(args, reader: MarkdownExampleReader):
	"""Extract and display examples."""
	sources = ["auto", "project", "manual"] if args.source == "all" else [args.source]
	
	for source in sources:
		examples = reader.get_examples(source, args.class_name, args.method)
		
		if not examples:
			continue
		
		if args.slug:
			examples = [ex for ex in examples if ex["slug"] == args.slug]
		
		if not examples:
			continue
		
		phase = SOURCE_TO_PHASE[source]
		print(f"=== {args.class_name}.{args.method} [{source}/{phase}] ===")
		print(f"{len(examples)} example(s)\n")
		
		for ex in examples:
			title = ex.get("title", "Example")
			print(f"--- {ex['slug']}: {title} ---")
			print(ex.get("code", "(no code)"))
			print()
			
			if args.show_metadata:
				metadata = ex.get("testMetadata")
				print(format_test_metadata(metadata))
				print()
	
	# Check if nothing was found at all
	found = False
	for source in sources:
		if reader.get_examples(source, args.class_name, args.method):
			found = True
			break
	
	if not found:
		print(f"No examples found for {args.class_name}.{args.method}")
		if args.slug:
			print(f"  (with slug filter: {args.slug})")
		return 1
	
	return 0


def cmd_add_metadata(args, reader: MarkdownExampleReader) -> int:
	"""Add or update testMetadata for an example in its .md source file."""
	md_path = reader.resolve_path(args.source, args.class_name, args.method)
	if md_path is None:
		print(Colors.red(f"[ERROR] No markdown file found for {args.class_name}.{args.method} in source '{args.source}'"))
		return 1
	
	# Verify slug exists in the file
	example = reader.get_example_by_slug(args.source, args.class_name, args.method, args.slug)
	if example is None:
		print(Colors.red(f"[ERROR] Slug '{args.slug}' not found in {md_path}"))
		available = reader.get_examples(args.source, args.class_name, args.method)
		if available:
			slugs = [ex["slug"] for ex in available]
			print(f"Available slugs: {', '.join(slugs)}")
		return 1
	
	# Build testMetadata
	testable = args.testable == "true"
	test_metadata = {"testable": testable}
	
	if not testable and args.skip_reason:
		test_metadata["skipReason"] = args.skip_reason
	
	if testable:
		# Parse verification arguments
		if args.verify_type == "log-output":
			try:
				values = json.loads(args.verify_values)
			except json.JSONDecodeError as e:
				print(Colors.red(f"[ERROR] Invalid JSON in --verify-values: {e}"))
				return 1
			test_metadata["verifyScript"] = {"type": "log-output", "values": values}
		
		elif args.verify_type == "REPL":
			try:
				checks = json.loads(args.verify_checks)
				if not isinstance(checks, list):
					print(Colors.red("[ERROR] --verify-checks must be a JSON array"))
					return 1
				for check in checks:
					if not isinstance(check, dict) or "expression" not in check or "value" not in check:
						print(Colors.red("[ERROR] Each REPL check must have 'expression' and 'value' fields"))
						return 1
			except json.JSONDecodeError as e:
				print(Colors.red(f"[ERROR] Invalid JSON in --verify-checks: {e}"))
				return 1
			
			if len(checks) == 1:
				test_metadata["verifyScript"] = {
					"type": "REPL",
					"expression": checks[0]["expression"],
					"value": checks[0]["value"]
				}
			else:
				test_metadata["verifyScript"] = checks
		
		elif args.verify_type == "expect-error":
			test_metadata["verifyScript"] = {
				"type": "expect-error",
				"errorMessage": args.verify_error_message
			}
	
	# Write to .md file
	success, message = update_metadata_in_markdown(Path(md_path), args.slug, test_metadata)
	
	if success:
		print(Colors.green(f"[OK] {message}"))
		return 0
	else:
		print(Colors.red(f"[ERROR] {message}"))
		return 1


def cmd_edit(args, reader: MarkdownExampleReader) -> int:
	"""Edit example code in its .md source file."""
	md_path = reader.resolve_path(args.source, args.class_name, args.method)
	if md_path is None:
		print(Colors.red(f"[ERROR] No markdown file found for {args.class_name}.{args.method} in source '{args.source}'"))
		return 1
	
	# Verify slug exists
	example = reader.get_example_by_slug(args.source, args.class_name, args.method, args.slug)
	if example is None:
		print(Colors.red(f"[ERROR] Slug '{args.slug}' not found in {md_path}"))
		available = reader.get_examples(args.source, args.class_name, args.method)
		if available:
			slugs = [ex["slug"] for ex in available]
			print(f"Available slugs: {', '.join(slugs)}")
		return 1
	
	success, message = update_code_in_markdown(Path(md_path), args.slug, args.code)
	
	if success:
		print(Colors.green(f"[OK] {message}"))
		return 0
	else:
		print(Colors.red(f"[ERROR] {message}"))
		return 1


def cmd_validate(args, reader: MarkdownExampleReader,
                 sidecar: SidecarResultStore) -> int:
	"""Validate examples via HISE REST API, record results to sidecar."""
	# Connect to HISE
	print("Connecting to HISE REST API...")
	api = HISEAPIClient(args.api_url)
	api.debug_timing = getattr(args, "timing", False)
	try:
		status = api.status()
		print(f"[OK] Connected to HISE {status['server']['version']}")
		print(f"     Project: {status['project']['name']}\n")
	except HISEConnectionError as e:
		print(Colors.red(f"[ERROR] {e}"))
		return 1
	except Exception as e:
		print(Colors.red(f"[ERROR] Unexpected error connecting to HISE: {e}"))
		return 1
	
	validator = SnippetValidator(api)
	sidecar.load()
	
	sources = ["auto", "project", "manual"] if args.source == "all" else [args.source]
	
	stats = {"tested": 0, "passed": 0, "failed": 0, "skipped": 0}
	failed_examples = []
	connection_lost = False
	
	for source in sources:
		if connection_lost:
			break
		
		# Determine which methods to test
		if args.method:
			methods_to_test = [args.method]
		else:
			methods_to_test = reader.list_methods(source, args.class_name)
		
		if not methods_to_test:
			continue
		
		phase = SOURCE_TO_PHASE[source]
		print(f"--- Validating {args.class_name} [{source}/{phase}] ---\n")
		
		for method_name in sorted(methods_to_test):
			if connection_lost:
				break
			
			examples = reader.get_examples(source, args.class_name, method_name)
			
			for ex in examples:
				slug = ex["slug"]
				
				if args.slug and slug != args.slug:
					continue
				
				key = sidecar.make_key(args.class_name, method_name, source, slug)
				
				label = f"{args.class_name}.{method_name}.{source}:{slug}"
				print(f"Testing {label}...", end=" ", flush=True)
				
				try:
					result = validator.test_example(ex)
				except HISEConnectionError as e:
					print(Colors.red("[CONNECTION LOST]"))
					print(Colors.red(f"\n  HISE connection lost during test: {e}"))
					connection_lost = True
					break
				
				sidecar.record(key, result)
				
				if result.get("tested"):
					stats["tested"] += 1
					if result.get("passed"):
						stats["passed"] += 1
						print(Colors.green("[PASS]"))
					else:
						stats["failed"] += 1
						failed_examples.append({
							"key": key,
							"label": label,
							"stage": result.get("stage"),
							"error": result.get("error"),
							"logs": result.get("logs", [])
						})
						print(Colors.red("[FAIL]"))
				else:
					stats["skipped"] += 1
					reason = result.get("reason", "")
					print(Colors.yellow(f"[SKIP] {reason}"))
	
	# Save sidecar (always - preserves results collected before any failure)
	sidecar.save()
	
	if connection_lost:
		saved_count = stats["tested"] + stats["skipped"]
		print(f"\nSaved {saved_count} result(s) to {sidecar.path} before connection loss.")
		print("Restart HISE and re-run to continue validation.")
	else:
		print(f"\nResults saved to {sidecar.path}")
	
	# Print report
	print("\n" + "=" * 70)
	print("VALIDATION REPORT")
	print("=" * 70)
	print(f"Tested:  {stats['tested']}")
	print(f"Passed:  {stats['passed']} ({stats['passed']/max(stats['tested'],1)*100:.1f}%)")
	print(f"Failed:  {stats['failed']}")
	print(f"Skipped: {stats['skipped']}")
	
	if failed_examples:
		print("\n" + "=" * 70)
		print("FAILED EXAMPLES")
		print("=" * 70)
		for fail in failed_examples:
			print(f"\n{fail['label']}")
			print(f"  Stage: {fail['stage']}")
			print(f"  Error: {fail['error']}")
			if fail.get("logs"):
				print(f"  Logs: {fail['logs']}")
	
	print("\n" + "=" * 70)
	
	if connection_lost:
		return 2
	return 0 if stats['failed'] == 0 else 1


# ---------------------------------------------------------------------------
# main()
# ---------------------------------------------------------------------------

def main():
	parser = argparse.ArgumentParser(
		description="Validate HISE API examples from Phase 1/2/3 markdown source files"
	)
	
	# Mode (mutually exclusive)
	mode = parser.add_mutually_exclusive_group(required=True)
	mode.add_argument("--coverage", action="store_true",
	                  help="Generate coverage report")
	mode.add_argument("--extract", action="store_true",
	                  help="Extract and display examples")
	mode.add_argument("--add-metadata", action="store_true",
	                  help="Add/update testMetadata for an example")
	mode.add_argument("--edit", action="store_true",
	                  help="Update code for an example")
	mode.add_argument("--validate", action="store_true",
	                  help="Run tests via HISE REST API")
	mode.add_argument("--shutdown", action="store_true",
	                  help="Shut down a running HISE instance and exit")
	
	# Common filters
	parser.add_argument("--source",
	                    choices=["auto", "project", "manual", "all"],
	                    help="Source phase: auto (phase1), project (phase2), manual (phase3), or all")
	parser.add_argument("--class", dest="class_name",
	                    help="Class/namespace name")
	parser.add_argument("--method",
	                    help="Method name")
	parser.add_argument("--all-classes", action="store_true",
	                    help="Coverage for all classes (--coverage only)")
	
	# Extract options
	parser.add_argument("--slug",
	                    help="Example slug (required for --edit/--add-metadata, optional filter for --extract)")
	parser.add_argument("--show-metadata", action="store_true",
	                    help="Include test metadata in --extract output")
	
	# Add-metadata options
	parser.add_argument("--testable", choices=["true", "false"],
	                    help="Whether example is testable")
	parser.add_argument("--verify-type", choices=["log-output", "REPL", "expect-error"],
	                    help="Verification type")
	parser.add_argument("--verify-values",
	                    help="JSON array of expected log values (log-output)")
	parser.add_argument("--verify-checks",
	                    help="JSON array of REPL checks [{expression, value}]")
	parser.add_argument("--verify-error-message",
	                    help="Expected error message pattern (expect-error)")
	parser.add_argument("--skip-reason",
	                    help="Free-form reason why example is not testable (used with --testable false)")
	parser.add_argument("--setup", default="",
	                    help="Setup script code (default: empty)")
	
	# Edit options
	parser.add_argument("--code",
	                    help="New code content (--edit only)")
	
	# Validate options
	parser.add_argument("--api-url", default="http://127.0.0.1:1900",
	                    help="HISE REST API URL (default: http://127.0.0.1:1900)")
	parser.add_argument("--timing", action="store_true",
	                    help="Print HTTP round-trip times for each API call")
	
	# Launch options
	parser.add_argument("--launch", action="store_true",
	                    help="Launch HISE before validation, shut down after (--validate only)")
	parser.add_argument("--keep-alive", action="store_true",
	                    help="Keep HISE running after validation (requires --launch)")
	parser.add_argument("--no-debug", action="store_true",
	                    help="Use HISE release build instead of Debug (default: Debug)")
	parser.add_argument("--port", type=int, default=1900,
	                    help="HISE REST API port (default: 1900)")
	
	args = parser.parse_args()
	
	# --- Argument validation ---
	
	# Standalone --shutdown needs no other args
	if args.shutdown:
		launcher = HISELauncher(debug=not args.no_debug, port=args.port)
		exit(0 if launcher.shutdown() else 1)
	
	# --source is required for all modes
	if not args.source:
		parser.error("--source is required")
	
	# --launch and --keep-alive only with --validate
	if args.launch and not args.validate:
		parser.error("--launch can only be used with --validate")
	if args.keep_alive and not args.launch:
		parser.error("--keep-alive requires --launch")
	
	# Derive api_url from --port if --api-url wasn't explicitly set
	if args.api_url == "http://127.0.0.1:1900" and args.port != 1900:
		args.api_url = f"http://127.0.0.1:{args.port}"
	
	if args.coverage:
		if not (args.class_name or args.all_classes):
			parser.error("--coverage requires --class or --all-classes")
	
	elif args.extract:
		if not args.class_name or not args.method:
			parser.error("--extract requires --class and --method")
	
	elif args.add_metadata:
		if not args.class_name or not args.method or not args.slug or not args.testable:
			parser.error("--add-metadata requires --class, --method, --slug, and --testable")
		if args.testable == "true":
			if not args.verify_type:
				parser.error("--testable true requires --verify-type")
			if args.verify_type == "log-output" and not args.verify_values:
				parser.error("--verify-type log-output requires --verify-values")
			if args.verify_type == "REPL" and not args.verify_checks:
				parser.error("--verify-type REPL requires --verify-checks")
			if args.verify_type == "expect-error" and not args.verify_error_message:
				parser.error("--verify-type expect-error requires --verify-error-message")
	
	elif args.edit:
		if not args.class_name or not args.method or not args.slug or not args.code:
			parser.error("--edit requires --class, --method, --slug, and --code")
	
	elif args.validate:
		if not args.class_name:
			parser.error("--validate requires --class")
	
	# --- Initialize reader and sidecar ---
	
	reader = MarkdownExampleReader()
	sidecar = SidecarResultStore()
	
	# --- Dispatch ---
	
	if args.coverage:
		return cmd_coverage(args, reader, sidecar)
	elif args.extract:
		return cmd_extract(args, reader)
	elif args.add_metadata:
		return cmd_add_metadata(args, reader)
	elif args.edit:
		return cmd_edit(args, reader)
	elif args.validate:
		launcher = None
		if args.launch:
			launcher = HISELauncher(debug=not args.no_debug, port=args.port)
			if not launcher.launch():
				return 3
			# Override api_url when launched
			if args.api_url == f"http://127.0.0.1:{args.port}":
				args.api_url = launcher.api_url
		
		try:
			return cmd_validate(args, reader, sidecar)
		finally:
			if launcher:
				launcher.cleanup(keep_alive=args.keep_alive)


if __name__ == "__main__":
	exit(main())

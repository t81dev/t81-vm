#!/usr/bin/env bash
set -euo pipefail

BASE_REF="${BASE_REF:-main}"
REMOTE_BASE="origin/${BASE_REF}"
CONTRACT_FILE="docs/contracts/vm-compatibility.json"
CHANGELOG_FILE="CHANGELOG.md"
OPCODE_FILE="include/t81/tisc/opcodes.hpp"
ABI_FILES=(
  "include/t81/vm/c_api.h"
  "include/t81/vm/vm.hpp"
)

if ! git show-ref --verify --quiet "refs/remotes/${REMOTE_BASE}"; then
  git fetch origin "${BASE_REF}:${BASE_REF}" >/dev/null 2>&1 || true
  if ! git show-ref --verify --quiet "refs/remotes/${REMOTE_BASE}"; then
    echo "warning: missing ${REMOTE_BASE}; skipping discipline check"
    exit 0
  fi
fi

changed="$(git diff --name-only "${REMOTE_BASE}...HEAD")"
contract_changed=0
changelog_changed=0
opcode_surface_changed=0
abi_surface_changed=0

if printf '%s\n' "${changed}" | rg -qx "${CONTRACT_FILE}"; then
  contract_changed=1
fi
if printf '%s\n' "${changed}" | rg -qx "${CHANGELOG_FILE}"; then
  changelog_changed=1
fi
if printf '%s\n' "${changed}" | rg -qx "${OPCODE_FILE}"; then
  opcode_surface_changed=1
fi
for abi_file in "${ABI_FILES[@]}"; do
  if printf '%s\n' "${changed}" | rg -qx "${abi_file}"; then
    abi_surface_changed=1
    break
  fi
done

if [[ "${contract_changed}" -eq 1 && "${changelog_changed}" -ne 1 ]]; then
  echo "error: ${CONTRACT_FILE} changed but ${CHANGELOG_FILE} was not updated"
  exit 1
fi
if [[ "${opcode_surface_changed}" -eq 1 && "${contract_changed}" -ne 1 ]]; then
  echo "error: ${OPCODE_FILE} changed but ${CONTRACT_FILE} was not updated"
  exit 1
fi
if [[ "${opcode_surface_changed}" -eq 1 && "${changelog_changed}" -ne 1 ]]; then
  echo "error: ${OPCODE_FILE} changed but ${CHANGELOG_FILE} was not updated"
  exit 1
fi
if [[ "${abi_surface_changed}" -eq 1 && "${contract_changed}" -ne 1 ]]; then
  echo "error: VM ABI surface changed but ${CONTRACT_FILE} was not updated"
  exit 1
fi
if [[ "${abi_surface_changed}" -eq 1 && "${changelog_changed}" -ne 1 ]]; then
  echo "error: VM ABI surface changed but ${CHANGELOG_FILE} was not updated"
  exit 1
fi

echo "contract discipline: ok"

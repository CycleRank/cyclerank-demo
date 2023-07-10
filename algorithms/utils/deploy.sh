#!/usr/bin/env bash
# shellcheck disable=SC2128
SOURCED=false && [ "$0" = "$BASH_SOURCE" ] || SOURCED=true

if ! $SOURCED; then
  set -euo pipefail
  IFS=$'\n\t'
fi

#################### help
function short_usage() {
  (>&2 echo \
"Usage:
  $(basename "$0") [options]")
}

function usage() {
  (>&2 short_usage )
  (>&2 echo \
"
Deploy the website to production.
The configuration is in <repo_base>/.environment

Options:
  -d                Enable debug output (implies -v).
  -e ENV_FILE       Environment file to read the configuration from.
  -n                Dry-run, does not deploy any file.
  -p                Push repo before deplyoing.
  -s                Pass --size-only option to rsync.
  -t                Tunnel connection (see .environment).
  -q                Suppress output (incompatible with --debug or --verbose).
  -v                Enable verbose output.
  -V                Show version information.
  -h                Show this help and exits.

Example:
  $(basename "$0")
")
}

version(){
  (>&2 echo \
"$(basename "$0") 0.2
copyright (c) 2023 Cristian Consonni
MIT License
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
")
}

DEBUG=false
VERBOSE=false
NFLAG=""
SFLAG=""
PUSH_REPO=false
TUNNEL=false
ENV_FILE=""
QUIET=false

while getopts ":denpstqvVh" opt; do
  case $opt in
    d)
      DEBUG=true
      ;;
    e)
      ENV_FILE="$OPTARG"
      ;;
    n)
      NFLAG="-n"
      ;;
    p)
      PUSH_REPO=true
      ;;
    s)
      SFLAG="--size-only"
      ;;
    t)
      TUNNEL=true
      ;;
    q)
      QUIET=true
      ;;
    v)
      VERBOSE=true
      ;;
    V)
      version
      exit 0
      ;;
    h)
      usage
      exit 0
      ;;
    \?)
      (>&2 echo "Invalid option: -$OPTARG" )
      usage
      exit 1
      ;;
    :)
      (>&2 echo "Option -$OPTARG requires an argument." )
      exit 1
      ;;
  esac
done

if ($DEBUG || $VERBOSE) && $QUIET; then
  (>&2 echo "Error: --quiet and --debug/--verbose options are incompatible.")
  usage
  exit 1
fi

# -d (debug) implies -v (verbose)
if $DEBUG; then { VERBOSE=true; } fi

#################### Utils
if $DEBUG; then
  echodebug() {
    (>&2 echo -en "[$(date '+%F %H:%M:%S')][debug]\\t" )
    (>&2 echo "$@" )
  }
 else
  echodebug() { true; }
 fi

 if $VERBOSE; then
  echoverbose() {
    (>&2 echo -en "[$(date '+%F %H:%M:%S')][info]\\t" )
    (>&2 echo "$@" )
  }
 else
  echoverbose() { true; }
 fi

if $QUIET; then
  mute_cmd() {
    "$@" >/dev/null
  }

  echoquiet() { true; }
else
  mute_cmd() { "$@"; }
  echoquiet() { echo "$@"; }
fi
####################

if [ -z "$NFLAG" ]; then
  echodebug "NFLAG: (unset)"
else
  echodebug "NFLAG: $NFLAG"
fi

if [ -z "$NFLAG" ]; then
  echodebug "SFLAG: (unset)"
else
  echodebug "SFLAG: $SFLAG"
fi

echodebug "PUSH_REPO: $PUSH_REPO"
echodebug "TUNNEL: $TUNNEL"

echoverbose ""

if $PUSH_REPO; then
  echoverbose ""
  echoverbose "Pushing git repo: git push origin master"
  echoverbose ""

  verbosity_flag='--quiet'
  if $VERBOSE; then
    verbosity_flag=''
  fi

  git push "$verbosity_flag" origin master
fi

# Set the environment by loading from the file .environment in the base
# directory
scriptdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
repo_basedir=$(cd "$scriptdir" && git rev-parse --show-toplevel)

if [ -z "$ENV_FILE" ]; then
  ENV_FILE="$repo_basedir/.environment"
fi

echodebug "ENV_FILE: $ENV_FILE"

# shellcheck disable=SC1090
source "$ENV_FILE"

if [[ "${DEPLOY_SERVER}" == localhost ]] || \
     [[ "${DEPLOY_SERVER}" == 127.0.0.1 ]]; then
  echoverbose "Deploying ${repo_basedir}/${DEPLOY_SOURCE_DIR} to "\
  "${DEPLOY_DEST_DIR}"
else
  echoverbose "Deploying ${repo_basedir}/${DEPLOY_SOURCE_DIR} to "\
  "${DEPLOY_ACCOUNT}@${DEPLOY_SERVER}:${DEPLOY_DEST_DIR}"
fi

if $TUNNEL; then
  echoverbose "---> tunneling through ${TUNNEL_SERVER}:${TUNNEL_PORT}"
fi

echoverbose -n "Clean up directory "
find . -type f -name '*.DS_Store' -ls -delete

if $VERBOSE; then
  echo "...done"
fi

# Transfer to server with rsync
echoverbose ""
echoverbose "Performing transfer to server"

# Ensure .deployignore exists
touch "${repo_basedir}/.deployignore"

verbosity_flag='-v'
if $QUIET; then
  verbosity_flag=''
fi

tunnel_option=''
tunnel_arg=''
if $TUNNEL; then
  tunnel_option='-e'
  tunnel_arg="ssh -p $TUNNEL_PORT"
  echoverbose "---> tunneling via $TUNNEL_SERVER:$TUNNEL_PORT"
fi

echoverbose ""

if [[ "${DEPLOY_SERVER}" == localhost ]] || \
     [[ "${DEPLOY_SERVER}" == 127.0.0.1 ]]; then
  echoquiet "--> transfer files from ${repo_basedir}/${DEPLOY_SOURCE_DIR} to "\
            "${DEPLOY_DEST_DIR}"
else
  echoquiet "--> transfer files from ${repo_basedir}/${DEPLOY_SOURCE_DIR} to "\
            "${DEPLOY_ACCOUNT}@${DEPLOY_SERVER}:${DEPLOY_DEST_DIR}"
fi

if $DEBUG; then
  set -x
fi

if [[ "${DEPLOY_SERVER}" == localhost ]] || \
     [[ "${DEPLOY_SERVER}" == 127.0.0.1 ]]; then
  # shellcheck disable=SC2086
  rsync -r --no-perms --ignore-times --checksum \
        ${verbosity_flag:-} ${NFLAG:-} ${SFLAG:-} \
        ${tunnel_option:-} ${tunnel_arg:-} \
        --delete \
        --exclude-from="${repo_basedir}/.deployignore" \
        "${repo_basedir}/${DEPLOY_SOURCE_DIR}" \
        "${DEPLOY_DEST_DIR}"

else
  # https://github.com/koalaman/shellcheck/wiki/SC2086
  # shellcheck disable=SC2086
  rsync -r --no-perms --ignore-times --checksum \
          ${verbosity_flag:-} ${NFLAG:-} ${SFLAG:-} \
          ${tunnel_option:-} ${tunnel_arg:-} \
          --delete \
          --exclude-from="${repo_basedir}/.deployignore" \
          "${repo_basedir}/${DEPLOY_SOURCE_DIR}" \
          "${DEPLOY_ACCOUNT}@${DEPLOY_SERVER}:${DEPLOY_DEST_DIR}"
fi

set +x

echoquiet ''

exit 0

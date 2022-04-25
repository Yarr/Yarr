_root=$(dirname ${BASH_SOURCE})/..
_root=$(cd ${_root}/> /dev/null 2>&1 && pwd)
export PYTHONPATH=${_root}/lib:${_root}/python:${PYTHONPATH}
unset _root

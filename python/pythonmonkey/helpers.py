# @file         helpers.py - Python->JS helpers for PythonMonkey
#               - typeof operator wrapper
#               - new operator wrapper
#
# @author       Wes Garland, wes@distributive.network
# @date         July 2023
#

from . import pythonmonkey as pm 
evalOpts = { 'filename': __file__, 'fromPythonFrame': True }

def typeof(jsval):
    """
    typeof function - wraps JS typeof operator
    """
    return pm.eval("""'use strict'; (
function pmTypeof(jsval) 
{
  return typeof jsval;
}
    )""", evalOpts)(jsval);

def new(ctor):
    """
    new function - emits function which wraps JS new operator, emitting a lambda which constructs a new
    JS object upon invocation.
    """
    if (typeof(ctor) == 'string'):
        ctor = pm.eval(ctor)

    newCtor = pm.eval("""'use strict'; (
function pmNewFactory(ctor)
{
  return function newCtor(args) {
    args = Array.from(args || []);
    return new ctor(...args);
  };
}
    )""", evalOpts)(ctor)
    return (lambda *args: newCtor(list(args)))

# List which symbols are exposed to the pythonmonkey module.
__all__ = [ "new", "typeof" ]

# Add the properties of globalThis (except eval) as exports:
globalThis = pm.eval('globalThis');

exports = pm.eval("""
Object.getOwnPropertyNames(globalThis)
.filter(prop => prop !== 'eval')
.filter(prop => prop[0] !== '_')
.filter(prop => Object.keys(globalThis).indexOf(prop) === -1)
""")

for index in range(0, int(exports.length) - 1):
    name = exports[index]
    globals().update({name: globalThis[name]})
    __all__.append(name)

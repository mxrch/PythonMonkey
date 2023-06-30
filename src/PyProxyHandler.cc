/**
 * @file PyProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for creating JS proxy objects. Used by DictType for object coercion
 * @version 0.1
 * @date 2023-04-20
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/PyProxyHandler.hh"

#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Conversions.h>
#include <js/Proxy.h>

#include <Python.h>

PyObject *idToKey(JSContext *cx, JS::HandleId id) {
  if (id.isSymbol() || id.isVoid()) {
    // TODO (Tom Tang): Revisit this once we have Symbol coecrion support
    Py_RETURN_NOTIMPLEMENTED;
  }

  JS::RootedValue idv(cx, js::IdToValue(id));
  return StrType(cx, JS::ToString(cx, idv)).getPyObject();
}

PyProxyHandler::PyProxyHandler(PyObject *pyObj) : js::BaseProxyHandler(NULL), pyObject(pyObj) {}

bool PyProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  PyObject *keys = PyDict_Keys(pyObject);
  for (size_t i = 0; i < PyList_Size(keys); i++) {
    PyObject *key = PyList_GetItem(keys, i);
    JS::RootedValue jsKey(cx, jsTypeFactory(cx, key));
    JS::RootedId jsId(cx);
    if (!JS_ValueToId(cx, jsKey, &jsId)) {
      // @TODO (Caleb Aikens) raise exception
    }
    if (!props.append(jsId)) {
      // @TODO (Caleb Aikens) raise exception
    }
  }
  return true;
}

bool PyProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::ObjectOpResult &result) const {
  PyObject *attrName = idToKey(cx, id);
  if (PyDict_DelItem(pyObject, attrName) < 0) {
    // @TODO (Caleb Aikens) raise exception
    return false;
  }
  return true;
}

bool PyProxyHandler::has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  return hasOwn(cx, proxy, id, bp);
}

bool PyProxyHandler::get(JSContext *cx, JS::HandleObject proxy,
  JS::HandleValue receiver, JS::HandleId id,
  JS::MutableHandleValue vp) const {
  PyObject *attrName = idToKey(cx, id);
  PyObject *p = PyDict_GetItemWithError(pyObject, attrName);
  if (!p) { // NULL if the key is not present
    vp.setUndefined(); // JS objects return undefined for nonpresent keys
  } else {
    vp.set(jsTypeFactory(cx, p));
  }
  return true;
}

bool PyProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::HandleValue v, JS::HandleValue receiver,
  JS::ObjectOpResult &result) const {
  JS::RootedValue *rootedV = new JS::RootedValue(cx, v);
  PyObject *attrName = idToKey(cx, id);
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  if (PyDict_SetItem(pyObject, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
    // @TODO (Caleb Aikens) raise exception
    return false;
  }
  // @TODO (Caleb Aikens) read about what you're supposed to do with receiver and result
  return true;
}

bool PyProxyHandler::enumerate(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

bool PyProxyHandler::hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  PyObject *attrName = idToKey(cx, id);
  *bp = PyDict_Contains(pyObject, attrName) == 1;
  return true;
}

bool PyProxyHandler::getOwnEnumerablePropertyKeys(
  JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

// @TODO (Caleb Aikens) implement this
void PyProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {}

bool PyProxyHandler::defineProperty(JSContext *cx, JS::HandleObject proxy,
  JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc,
  JS::ObjectOpResult &result) const {
  if (desc.hasConfigurable() && desc.configurable()) {}

  // JS::RootedValue *rootedV = new JS::RootedValue(cx, v);
  // PyObject *attrName = idToKey(cx, id);
  // JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  // if (PyDict_SetItem(pyObject, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
  //   // @TODO (Caleb Aikens) raise exception
  //   return false;
  // }
  // // @TODO (Caleb Aikens) read about what you're supposed to do with receiver and result
  // return true;
}

bool PyProxyHandler::getPrototypeIfOrdinary(JSContext *cx, JS::HandleObject proxy,
  bool *isOrdinary,
  JS::MutableHandleObject protop) const {
  // We don't have a custom [[GetPrototypeOf]]
  *isOrdinary = true;
  protop.set(js::GetStaticPrototype(proxy));
  return true;
}

bool PyProxyHandler::preventExtensions(JSContext *cx, JS::HandleObject proxy,
  JS::ObjectOpResult &result) const {
  result.succeed();
  return true;
}

bool PyProxyHandler::isExtensible(JSContext *cx, JS::HandleObject proxy,
  bool *extensible) const {
  *extensible = false;
  return true;
}
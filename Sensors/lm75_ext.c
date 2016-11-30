/*
 * @author 	Alexander Rüedlinger <a.rueedlinger@gmail.com>
 * @date 	13.03.2015
 *
 * Python bindings for the lm75 driver written in C.
 *
 */

#include <Python.h>
#include <structmember.h>
#include "lm75.h"

typedef struct {
	PyObject_HEAD
	void *lm75;
} LM75_Object;



static void LM75_dealloc(LM75_Object *self) {
	lm75_close(self->lm75);
	self->lm75 = NULL;
	self->ob_type->tp_free((PyObject*)self);
}



static PyObject *LM75_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	LM75_Object *self;
	self = (LM75_Object *) type->tp_alloc(type, 0);
	return (PyObject *) self;
}



static int LM75_init(LM75_Object *self, PyObject *args, PyObject *kwds) {
	int address;
	const char *i2c_device;
	static char *kwlist[] = {"address", "i2c_devcie", NULL};

	if(!PyArg_ParseTupleAndKeywords(args, kwds, "is", kwlist, &address, &i2c_device))
		return -1;

	if(i2c_device) {
		self->lm75 = lm75_init(address, i2c_device);
		if(self->lm75 == NULL) {
			PyErr_SetString(PyExc_RuntimeError, "Cannot initialize sensor. Run program as root and check i2c device / address.");
			return -1;
		}
	}
	return 0;
}



static PyObject *LM75_temperature(LM75_Object *self) {
	PyObject *result;
	double temperature = lm75_temperature(self->lm75);
	result = PyFloat_FromDouble(temperature);
	return result;
}



static PyMethodDef LM75_methods[] = {
	{"temperature", (PyCFunction) LM75_temperature, METH_NOARGS, "Returns a temperature value"},
	{NULL}  /* Sentinel */
};



static PyMemberDef LM75_members[] = {
    {NULL}  /* Sentinel */
};



static PyTypeObject LM75_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"tentacle_pi.LM75",             /*tp_name*/
	sizeof(LM75_Object),             /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)LM75_dealloc, /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"LM75 objects",           /* tp_doc */
	0,		               /* tp_traverse */
	0,		               /* tp_clear */
	0,		               /* tp_richcompare */
	0,		               /* tp_weaklistoffset */
	0,		               /* tp_iter */
	0,		               /* tp_iternext */
	LM75_methods,             /* tp_methods */
	LM75_members,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)LM75_init,      /* tp_init */
	0,                         /* tp_alloc */
	LM75_new,                 /* tp_new */
};



static PyMethodDef module_methods[] = {
	{NULL}  /* Sentinel */
};



PyMODINIT_FUNC initLM75(void) {
	PyObject *m;

	if(PyType_Ready(&LM75_Type) < 0)
		return;

	m = Py_InitModule3("LM75", module_methods, "LM75 extension module");

	if(m == NULL)
		return;

	Py_INCREF(&LM75_Type);
	PyModule_AddObject(m, "LM75", (PyObject *)&LM75_Type);
}

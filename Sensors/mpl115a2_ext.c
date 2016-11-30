/*
 * @author 	Alexander Rüedlinger <a.rueedlinger@gmail.com>
 * @date 	9.03.2015
 *
 * Python bindings for the MPL115A2 driver written in C.
 *
 */

#include <Python.h>
#include <structmember.h>
#include "mpl115a2.h"

typedef struct {
	PyObject_HEAD
	void *mpl115a2;
} MPL115A2_Object;



static void MPL115A2_dealloc(MPL115A2_Object *self) {
	mpl115a2_close(self->mpl115a2);
	self->mpl115a2 = NULL;
	self->ob_type->tp_free((PyObject*)self);
}



static PyObject *MPL115A2_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	MPL115A2_Object *self;
	self = (MPL115A2_Object *) type->tp_alloc(type, 0);
	return (PyObject *) self;
}



static int MPL115A2_init(MPL115A2_Object *self, PyObject *args, PyObject *kwds) {
	int address;
	const char *i2c_device;
	static char *kwlist[] = {"address", "i2c_devcie", NULL};

	if(!PyArg_ParseTupleAndKeywords(args, kwds, "is", kwlist, &address, &i2c_device))
		return -1;

	if(i2c_device) {
		self->mpl115a2 = mpl115a2_init(address, i2c_device);
    if(self->mpl115a2 == NULL) {
			PyErr_SetString(PyExc_RuntimeError, "Cannot initialize sensor. Run program as root and check i2c device / address.");
			return -1;
		}
  }

	return 0;
}



static PyObject *MPL115A2_temperature(MPL115A2_Object *self) {
	PyObject *result;
	double temperature = mpl115a2_temperature(self->mpl115a2);
	result = PyFloat_FromDouble(temperature);
	return result;
}



static PyObject *MPL115A2_pressure(MPL115A2_Object *self) {
	PyObject *result;
	double pressure = mpl115a2_pressure(self->mpl115a2);
	result = PyFloat_FromDouble(pressure);
	return result;
}



static PyObject *MPL115A2_sense(MPL115A2_Object *self) {
	float temperature = 0, pressure = 0;
	mpl115a2_read_data(self->mpl115a2, &temperature, &pressure);
	return Py_BuildValue("(ff)", temperature, pressure);
}



static PyMethodDef MPL115A2_methods[] = {
	{"temperature", (PyCFunction) MPL115A2_temperature, METH_NOARGS, "Returns a temperature value"},
	{"pressure", (PyCFunction) MPL115A2_pressure, METH_NOARGS, "Returns a pressure value"},
	{"sense", (PyCFunction) MPL115A2_sense, METH_NOARGS, "Returns a (pressure, temperature) tuple"},
	{NULL}  /* Sentinel */
};



static PyMemberDef MPL115A2_members[] = {
    {NULL}  /* Sentinel */
};



static PyTypeObject MPL115A2_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"tentacle_pi.MPL115A2",             /*tp_name*/
	sizeof(MPL115A2_Object),             /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)MPL115A2_dealloc, /*tp_dealloc*/
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
	"MPL115A2 objects",           /* tp_doc */
	0,		               /* tp_traverse */
	0,		               /* tp_clear */
	0,		               /* tp_richcompare */
	0,		               /* tp_weaklistoffset */
	0,		               /* tp_iter */
	0,		               /* tp_iternext */
	MPL115A2_methods,             /* tp_methods */
	MPL115A2_members,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)MPL115A2_init,      /* tp_init */
	0,                         /* tp_alloc */
	MPL115A2_new,                 /* tp_new */
};



static PyMethodDef module_methods[] = {
	{NULL}  /* Sentinel */
};



PyMODINIT_FUNC initMPL115A2(void) {
	PyObject *m;

	if(PyType_Ready(&MPL115A2_Type) < 0)
		return;

	m = Py_InitModule3("MPL115A2", module_methods, "MPL115A2 extension module");

	if(m == NULL)
		return;

	Py_INCREF(&MPL115A2_Type);
	PyModule_AddObject(m, "MPL115A2", (PyObject *)&MPL115A2_Type);
}

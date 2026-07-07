/************************************************************************
       -= SEER - C-Scripting engine v 0.3a =-
State      : under implementation
Author     : Przemyslaw Podsiadly
             email: ppodsiad@elka.pw.edu.pl
             WWW:   http://home.elka.pw.edu.pl/~ppodsiad
File:script.c
Desc:debug & initialization & other
************************************************************************/
#define INTERNAL_FILE
#include "seer.h"
#include "internal.h"
#include <sys/stat.h>

#define ToCodeINT(script) ((int*)script)

int scErrorNo = scOK;
char scErrorMsg[1024] = "";
char scErrorLine[1024] = "";

bool SeeR_Inited = false;
bool scIgnore_Rights = false;
int scParamCount;
scInstance *scActual_Instance = NULL;
int abort_executor;
int aborted_ip;
char scScript_Title[128] = "Untitled";
char scScript_Author[128] = "Anonymous";
char *scScript_SeeR =
	"SeeR (" SeeR_VERSION_STR
	") scripting language by Theur - Przemyslaw Podsiadly " SeeR_DATE_STR;
int (*scProgressCallback) (int percent) = NULL;

// scFunctionDispatcher scUserFunctionDispatcher[SC_MAX_FUNCTION_DISPATCHERS];

#ifdef DEBUG
FILE *debfile = NULL;
int debug_flag = 0;
#endif

// //from callasm.c:
// int callfunc(void *func, void *params, int count);
// double callfunc_d(void *func, void *params, int count);
// int callmember(void *func, void *params, int count);
// double callmember_d(void *func, void *params, int count);

#ifdef SC_MSVC
extern int _msvc_regEDX;
#endif
// void scStandardDispatcher
// 	(int *result, void *function, int *params, int paramcount,
// 	 unsigned int options) {deb0("Dispatcher ");
// 	if ((options & scDispatch_Struct)) {
// 		deb0("struct ");
// #ifdef SC_MSVC
// 		//TODO:test
// 		//ignore int*result (points to a0)
// 		switch ((options & scDispatch_Struct) >> 22) {
// 			{
// 		case 2:				//1 byte struct
// 		case 3:				//2 bytes struct
// 		case 4:				//4 bytes struct
// 				result = (int *) *(params++);
// 				paramcount -= 4;
// 				break;
// 		case 5:				//8 bytes struct
// 				//options|=scDispatch_Double;
// 				result = (int *) *(params++);
// 				paramcount -= 4;
// 				break;
// 		default:;				//nothing to do
// 			}
// 		}
// #endif							//SC_MSVC
// 	}

// 	if (!(options & scDispatch_Member)) {	//not a class-member!
// 		if (!(options & scDispatch_Double)) {
// 			deb0("int ");
// 			*result = callfunc(function, params, paramcount);
// 		} else {
// 			deb0("double ");
// 			debflush();
// 			(*((double *) result)) = callfunc_d(function,
// 												params, paramcount);
// 			deb0("Done.\n");
// 			debflush();
// 		}
// 	} else {					//a class-member!
// 		if (!(options & scDispatch_Double)) {
// 			*result = callmember(function, params, paramcount);
// 		} else {
// 			(*((double *) result)) = callmember_d(function,
// 												  params, paramcount);
// 		}
// 	}
// #ifdef SC_MSVC
// 	if ((options & scDispatch_Struct)
// 		&& ((options & scDispatch_Struct) >> 22) == 5)	//8byte struct
// 	{
// 		*(result + 1) = _msvc_regEDX;
// 	}
// #endif
// }

// void scCDispatcher
// 	(int *result, void *function, int *params, int paramcount,
// 	 unsigned int options) {
// 	if (options & scDispatch_Member) {	//C++ call - convert to typical C call, so 'this' is the first param.
// 		options ^= scDispatch_Member;	//erase member flag
// 	}
// 	//call the default dispatcher
// 	scStandardDispatcher(result, function, params, paramcount, options);
// }

int scSeeR_Get(int subject, ...)
{
	// int i;
	va_list va;
	va_start(va, subject);
	switch (subject) {
	case scSeeR_Version:
		return SeeR_VERSION;

	case scSeeR_Build:
#ifdef DEBUG
		return scSeeR_Build_Debug;
#else
		return scSeeR_Build_Release;
#endif

	case scSeeR_LinesCompiled:
		return act.lines_compiled;

	case scSeeR_Debugging:
#ifdef DEBUG
		return (debug_flag);
#else
		return 0;
#endif

	// case scSeeR_Dispatcher:
	// 	i = va_arg(va, int);
	// 	if (i < 0 || i >= SC_MAX_FUNCTION_DISPATCHERS)
	// 		return 0;
	// 	return (int) scUserFunctionDispatcher[i];

	case scSeeR_ProgressCallback:
		return (int) scProgressCallback;

	case scSeeR_ParamCount:
		return 0;
	case scSeeR_ExprOpcode:
		return (act.options & optEXPR) ? true : false;
	case scSeeR_StackSize:
		return (act.stacksize);
	}
	return 0;
}

bool scSeeR_Set(int subject, ...)
{
	int i = 0;
	void *ptr;
	va_list va;
	va_start(va, subject);
	switch (subject) {
	case scSeeR_Debugging:
#ifdef DEBUG
		i = va_arg(va, int);
		debug_flag = (bool) i;
#endif
		return true;
	case scSeeR_OpenDebug:
		scOpen_Debug(va_arg(va, char *));
		return true;
	case scSeeR_CloseDebug:
		scClose_Debug();
		return true;
	// case scSeeR_Dispatcher:
	// 	i = va_arg(va, int);
	// 	ptr = va_arg(va, void *);
	// 	if (i < 0 || i >= SC_MAX_FUNCTION_DISPATCHERS)
	// 		return false;
	// 	scUserFunctionDispatcher[i] = ptr;
	// 	return true;
	case scSeeR_ProgressCallback:
		ptr = va_arg(va, void *);
		scProgressCallback = ptr;
		return true;
	case scSeeR_ExprOpcode:
		i = va_arg(va, int);
		if (!i) {
			if (act.options & optEXPR)
				act.options ^= optEXPR;
		} else {
			if (!(act.options & optEXPR))
				act.options ^= optEXPR;
		}
		return true;
	case scSeeR_StackSize:
		act.stacksize = va_arg(va, int);
		return true;
	}
	return false;
}

// void scExport_Stdlib();

void scInit_SeeR(void)
{
	// int i;

	if (SeeR_Inited)
		return;

	SeeR_Inited = true;
	act.during = duringIDLE;

	// for (i = 0; i < SC_MAX_FUNCTION_DISPATCHERS; i++)
	// 	scUserFunctionDispatcher[i] = NULL;

	// scSeeR_Set(scSeeR_Dispatcher, 0, scStandardDispatcher);
	// scSeeR_Set(scSeeR_Dispatcher, 1, scCDispatcher);
	scSeeR_Set(scSeeR_ExprOpcode, 0);	//faster
	scSeeR_Set(scSeeR_StackSize, 4000);

	if (!scGet_Scheduler())
		scSet_Scheduler(NULL);
	scAdd_Internal_Header("Instances", hdrInstances);
	// scExport_Stdlib();
//Instances imports:
	scAddExtSym(scKill_My_Forked_Instances);
	scAddExtSym(scKill_Forked);
	scAddExtSym(scGet_Forked_Status);
	scAddExtSym(scGet_Actual_Priority);

//Done.
}

char *scGet_Title(scScript s)
{
	if (s == NULL) {
		return "<none>";
	}
	if (ToCodeINT(s)[0] != CHARS2INT('S', 'e', 'e', 'R')) {
		return "Not a SeeR script!\n";
	}
	if (ToCodeINT(s)[1] == CHARS2INT('V', 'C', 'P', 'U')) {
		return (char *)ToCodeINT(s)[hdrHeaderSizeOf];
	}
	return "Not a VCPU SeeR script!\n";
}

char *scGet_Author(scScript s)
{
	if (s == NULL) {
		return "<none>";
	}
	if (ToCodeINT(s)[0] != CHARS2INT('S', 'e', 'e', 'R')) {
		return "Not a SeeR script!\n";
	}
	if (ToCodeINT(s)[1] == CHARS2INT('V', 'C', 'P', 'U')) {
		char *p = (char *)ToCodeINT(s)[hdrHeaderSizeOf];
		return p + strlen(p) + 1;
	}
	return "Not a VCPU SeeR script!\n";
}

char *scGet_Instance_Title(scInstance * I)
{
	if (!I)
		return NULL;
	return scGet_Title(I->code);
}

scInstance *scGet_This()
{
//<ERASED> scKernelOnly(NULL);
	return scActual_Instance;
}

int scGet_Script_Size(scScript scrpt)
{
//<ERASED> scKernelOnly(0);
	if (!scrpt)
		return 0;
	if (*((int *) scrpt) != CHARS2INT('S', 'e', 'e', 'R'))
		return 0;
	return ((int *) scrpt)[3];
}

int STfilesize(const char *s)
{
	struct stat l;
	stat(s, &l);
	return l.st_size;
}

scScript scLoad_Script(char *s)
{
	int size;
	FILE *file;
	scScript script;
//<ERASED> scKernelOnly(NULL);
	size = STfilesize(s);
	file = fopen(s, "rb");
	if (!file)
		return NULL;
	script = malloc(size);
	if (fread(script, size, 1, file) == -1) {
		fclose(file);
		free(script);
		return NULL;
	};
	fclose(file);
	if (scGet_Script_Size(script) != size) {
		free(script);
		return NULL;
	};
	return script;
}

#ifdef DEBUG
void scToggle_Debug(bool debon)
{
	debug_flag = debon;
}

void scOpen_Debug(char *filename)
{
	scInit_SeeR();
	debfile = fopen(filename, "w");
	debug_flag = true;
	deb0("SeeRC debugger data:\n");
}

void scClose_Debug() //also turns debuggin offULL);l C call, so 'this' is the first param.***/
{
	debug_flag = 0;
	if (!debfile)
		return;
	fclose(debfile);
	debfile = NULL;
}
#else
void scToggle_Debug(bool debon)
{
}
void scOpen_Debug(char *filename)
{
}
void scClose_Debug()
{
}
#endif

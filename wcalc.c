#include "wcalc.h"

#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

/* dear diary,
 * this is the type of nodes we're dealing with.
 * there are nodes with 0 arguments (values), 1 argument (unary operators) and
 * nodes with 2 arguments (binary operators).
 */
typedef enum {
	NT_ARG0,
	NT_ARG1,
	NT_ARG2
} NodeType;

/* dear diary,
 * this is the type of unary operators. most of them should be obvious;
 * ld = log 2, ln = log e, lg = log 10.
 */
typedef enum {
	St1_NEG,
	St1_ABS,
	St1_SQRT,
	St1_EXP,
	St1_LD,
	St1_LN,
	St1_LG,
	St1_SIN,
	St1_COS,
	St1_TAN,
	St1_ASIN,
	St1_ACOS,
	St1_ATAN
} Subtype1;

/* dear diary,
 * this is the type of binary operators.
 */
typedef enum {
	St2_ADD,
	St2_SUB,
	St2_MUL,
	St2_DIV,
	St2_MOD,
	St2_POW,
	St2_LOG
} Subtype2;

/* dear diary,
 * this is the precedence table for all operators. note that all unary
 * operators have the same precedence (PREC_UNOP). even precedence levels mean
 * left associative, odd numbers right associative.
 * PREC_MAX_ must be greater than the others and even.
 */
enum {
	PREC_ADD  = 0,
	PREC_SUB  = PREC_ADD,
	PREC_MUL  = 2,
	PREC_DIV  = PREC_MUL,
	PREC_MOD  = PREC_DIV,
	PREC_POW  = 3,
	PREC_LOG  = 5,
	PREC_UNOP = 7,
	PREC_MAX_ = 8
};

/* dear diary,
 * this is the type of argumentless nodes.
 */
typedef struct {
	double val;
} Node0;

/* dear diary,
 * this is the type of nodes taking one argument.
 */
typedef struct {
	long precedence;
	Subtype1 type;
} Node1;

/* dear diary,
 * this is the type of nodes taking two arguments.
 * caution: precedence must be the first element in this type and Node1 above!
 * insert() relies on this (yay hax).
 */
typedef struct {
	long precedence;
	Subtype2 type;
} Node2;

/* dear diary,
 * this is the complete node type. all nodes share type information and two
 * child pointers. actual instance information is stored in the aux union.
 */
typedef struct Node Node;
struct Node {
	NodeType type;
	Node *left, *right;
	union {
		Node0 n0;
		Node1 n1;
		Node2 n2;
	} aux;
};

/* dear diary,
 * this is the evaluator. it's a straightforward recursive function.
 */
static double eval(const Node *p) {

	switch (p->type) {

		case NT_ARG0:
			return p->aux.n0.val;

		case NT_ARG1: {
			double x = eval(p->right);
			switch (p->aux.n1.type) {
				case St1_NEG:  return -x;
				case St1_ABS:  return fabs(x);
				case St1_SQRT: return sqrt(x);
				case St1_EXP:  return exp(x);
				case St1_LD:   return log(x) / log(2.0);
				case St1_LN:   return log(x);
				case St1_LG:   return log10(x);
				case St1_SIN:  return sin(x);
				case St1_COS:  return cos(x);
				case St1_TAN:  return tan(x);
				case St1_ASIN: return asin(x);
				case St1_ACOS: return acos(x);
				case St1_ATAN: return atan(x);
			}
			break;
		}

		case NT_ARG2: {
			double x = eval(p->left), y = eval(p->right);
			switch (p->aux.n2.type) {
				case St2_ADD: return x + y;
				case St2_SUB: return x - y;
				case St2_MUL: return x * y;
				case St2_DIV: return x / y;
				case St2_MOD: return fmod(x, y);
				case St2_POW: return pow(x, y);
				case St2_LOG: return log(y) / log(x);
			}
			break;
		}

	}

	/* we shouldn't get here */
	return -666.0;
}

/* dear diary,
 * this is a wrapper for <ctype.h> functions. they all have undefined behavior
 * when the value of their argument is not EOF and does not fit in an unsigned
 * char; i.e. passing negative values (and char is usually a signed type) is
 * an error. this function casts c to an unsigned char. happiness ensues.
 */
static int ctype(int (*f)(int), char c) {
	return f((unsigned char)c);
}

/* dear diary,
 * this function simply skips all initial whitespace in s.
 */
static void skipws(const char **s) {
	while (ctype(isspace, **s)) {
		++*s;
	}
}

/* dear diary,
 * these are our EFOO-style error macros. we can't use Exxx because those names
 * are reserved for <errno.h>.
 */
#define FAIL_NOMEM "Out of memory"
#define FAIL_NOVAL "Not enough mana"
#define FAIL_NVAL  "Too many operands"
#define FAIL_HUH   "Lexical error"

/* dear diary,
 * this is a simple allocation abstraction. it contains a pointer to storage,
 * a size field (that's how many elements are addressable), and a length (how
 * many elements are currently in use). Oh, and I'm doing this with macros
 * because I need it for different types and C lacks templates.
 */
#define ALLOC(T) Alloc_ ## T
#define DEFINE_ALLOC(T) typedef struct { T (*store); size_t len, size; } ALLOC(T)

DEFINE_ALLOC(Node);
DEFINE_ALLOC(double);
typedef Node **ppNode;
DEFINE_ALLOC(ppNode);

/* dear diary,
 * this is a helper function that inserts a new operator node into the current tree.
 * leaves stores pointers to "unfinished" ends of the tree. this is where the
 * values will be inserted later.
 */
static void insert(Node **proot, Node *p, ALLOC(ppNode) *leaves) {
	while (
		*proot && (
			p->aux.n2.precedence > (*proot)->aux.n2.precedence || (
				p->aux.n2.precedence % 2 &&
				p->aux.n2.precedence == (*proot)->aux.n2.precedence
			)
		)
	) {
		proot = &(*proot)->right;
	}
	if (*proot) {
		if (p->type == NT_ARG1) {
			p->right = *proot;
		} else {
			leaves->store[leaves->len++] = &p->right;
			p->left = *proot;
		}
	} else {
		if (p->type == NT_ARG1) {
			leaves->store[leaves->len - 1] = &p->right;
		} else {
			leaves->store[leaves->len - 1] = &p->left;
			leaves->store[leaves->len++] = &p->right;
		}
	}
	*proot = p;
}

/* dear diary,
 * horrible helper macros follow. they're only used in the parser function. I
 * can't just make them functions because they use "return" to signal failure.
 */

/* check for at least one unused entry in p */
#define CHECK(p) do { \
	if ((p)->len >= (p)->size) { \
		return FAIL_NOMEM; \
	} \
} while (0)

/* add an item to the end of p */
#define PUSH_MEM(p, v) do { \
	CHECK(p); \
	(p)->store[(p)->len++] = (v); \
} while (0)

/* store a pointer to the next free element of p in *a.
 * p must be an ALLOC(Node). set left and right to NULL as a side effect.
 */
#define GET_NODE(a, p) do { \
	CHECK(p); \
	*(a) = &(p)->store[(p)->len++]; \
	(*(a))->left = (*(a))->right = NULL; \
} while (0)

/* the most complex macro: allocate a new node (using the storage in m), set
 * its type to t (and subtype to st), precedence to prec, and insert it into
 * the tree specified by proot. pp is the stack of value pointers used by
 * insert(). (sel is either n1 or n2, depending on t.)
 */
#define MAKE_OP(proot, pp, m, t, sel, st, prec) do { \
	Node *p_; \
	GET_NODE(&p_, m); \
	p_->type = (t); \
	p_->aux.sel.type = (st); \
	p_->aux.sel.precedence = (prec); \
	CHECK(pp); \
	insert(proot, p_, pp); \
} while (0)

/* wrappers for unary and binary node creation. root, mlocs and mnode are
 * local variables in parse() below. yes, this breaks abstraction, but it's
 * so convenient!
 */
#define MAKE_UNOP_(st, prec)  MAKE_OP(&root, mlocs, mnode, NT_ARG1, n1, st, prec)
#define MAKE_BINOP_(st, prec) MAKE_OP(&root, mlocs, mnode, NT_ARG2, n2, st, prec)

/* even more abstract wrapping. this one relies on the fact that the names for
 * node types and precedence values are the same, minus prefixes. paren is a
 * local variable in parse(), which contains the current nesting level of
 * parentheses.
 */
#define MAKE_UNOP(type)  MAKE_UNOP_ (St1_ ## type, paren * PREC_MAX_ + PREC_UNOP)
#define MAKE_BINOP(type) MAKE_BINOP_(St2_ ## type, paren * PREC_MAX_ + PREC_ ## type)

/* allocate a new node from m, store the pointer in *pp, set its type to value
 * and copy v into it.
 */
#define MAKE_VAL(pp, m, v) do { \
	GET_NODE(pp, m); \
	(*pp)->type = NT_ARG0; \
	(*pp)->aux.n0.val = (v); \
} while (0)

/* succeed in parsing a token of length n. s is a local variable in parse()
 * containing the current position in the source string. we can't use do-while
 * because we need continue here.
 */
#define NEXT(n) if (1) { s += (n); continue; } else ((void)0)

/* dear diary,
 * this is the parser. it takes a place to store a pointer to the resulting
 * tree, a source string, and storage tables to use for parsing. the return
 * value is NULL for success or a pointer to a static error message for
 * failure.
 */
static const char *parse(
	Node **proot,
	const char *s,
	ALLOC(Node) *mnode,
	ALLOC(double) *mvals,
	ALLOC(ppNode) *mlocs
) {
	Node *root = NULL;
	long paren = 0;

	PUSH_MEM(mlocs, &root);

	while (skipws(&s), *s) {
		if (ctype(isdigit, *s) || (*s == '.' && ctype(isdigit, s[1]))) {
			char *tmp;
			/* we need tmp because &s has the wrong type: const char **
			 * instead of char ** as required by strtod().
			 */
			PUSH_MEM(mvals, strtod(s, &tmp));
			s = tmp;
			continue;
		}

		/* this is basically an inlined unrolled trie.
		 * it looks relatively sane, thanks to macros.
		 */
		switch (s[0]) {

			case '(':
				++paren;
				NEXT(1);

			case ')':
				--paren;
				NEXT(1);

			case '+':
				MAKE_BINOP(ADD);
				NEXT(1);

			case '-':
				MAKE_BINOP(SUB);
				NEXT(1);

			case '*':
				switch (s[1]) {
					case '*':
						MAKE_BINOP(POW);
						NEXT(2);
					default:
						MAKE_BINOP(MUL);
						NEXT(1);
				}
				break;

			case '/':
				MAKE_BINOP(DIV);
				NEXT(1);

			case '%':
				MAKE_BINOP(MOD);
				NEXT(1);

			case '^':
				MAKE_BINOP(POW);
				NEXT(1);

			case '~':
			case '_':
				MAKE_UNOP(NEG);
				NEXT(1);

			case 'a':
				switch (s[1]) {
					case 'b':
						if (s[2] == 's') {
							MAKE_UNOP(ABS);
							NEXT(3);
						}
						break;
					case 'c':
						if (s[2] == 'o' && s[3] == 's') {
							MAKE_UNOP(ACOS);
							NEXT(4);
						}
						break;
					case 's':
						if (s[2] == 'i' && s[3] == 'n') {
							MAKE_UNOP(ASIN);
							NEXT(4);
						}
						break;
					case 't':
						if (s[2] == 'a' && s[3] == 'n') {
							MAKE_UNOP(ATAN);
							NEXT(4);
						}
						break;
				}
				break;

			case 'c':
				if (s[1] == 'o' && s[2] == 's') {
					MAKE_UNOP(COS);
					NEXT(3);
				}
				break;

			case 'e':
				switch (s[1]) {
					case 'x':
						if (s[2] == 'p') {
							MAKE_UNOP(EXP);
							NEXT(3);
						}
						break;

					default:
						PUSH_MEM(mvals, exp(1.0));
						NEXT(1);
				}
				break;

			case 'l':
				switch (s[1]) {
					case 'd':
						MAKE_UNOP(LD);
						NEXT(2);
					case 'g':
						MAKE_UNOP(LG);
						NEXT(2);
					case 'n':
						MAKE_UNOP(LN);
						NEXT(2);
					case 'o':
						if (s[2] == 'g') {
							MAKE_BINOP(LOG);
							NEXT(3);
						}
						break;
				}
				break;

			case 'n':
				if (s[1] == 'e' && s[2] == 'g') {
					MAKE_UNOP(NEG);
					NEXT(3);
				}
				break;

			case 'p':
				if (s[1] == 'i') {
					PUSH_MEM(mvals, 2.0 * acos(0.0));
					NEXT(2);
				}
				break;

			case 'r':
				if (s[1] == 'a' && s[2] == 'n' && s[3] == 'd') {
					PUSH_MEM(mvals, rand() / (RAND_MAX + 1.0));
					NEXT(4);
				}
				break;

			case 's':
				switch (s[1]) {
					case 'i':
						if (s[2] == 'n') {
							MAKE_UNOP(SIN);
							NEXT(3);
						}
						break;
					case 'q':
						if (s[2] == 'r' && s[3] == 't') {
							MAKE_UNOP(SQRT);
							NEXT(4);
						}
						break;
				}
				break;

			case 't':
				if (s[1] == 'a' && s[2] == 'n') {
					MAKE_UNOP(TAN);
					NEXT(3);
				}
				break;

		}

		/* if we get here, s doesn't point to a valid token */
		return FAIL_HUH;
	}

	if (mvals->len > mlocs->len) {
		return FAIL_NVAL;
	}
	if (mvals->len < mlocs->len) {
		return FAIL_NOVAL;
	}

	{
		size_t i;
		for (i = 0; i < mlocs->len; ++i) {
			MAKE_VAL(mlocs->store[i], mnode, mvals->store[i]);
		}
	}

	*proot = root;
	return NULL;
}

/* dear diary,
 * this macro returns the size of an array.
 */
#define COUNTOF(a) (sizeof (a) / sizeof *(a))

/* dear diary,
 * this is the maximum size of the three allocation tables (one for the actual
 * tree nodes, one for temporarily storing values, one for temporarily storing
 * pointers to tree leaves). 1024 should be more than enough for IRC with its
 * 512 char line length limit.
 */
enum {MAXMEM = 1024};

/* dear diary,
 * this is the public entry point. it takes the address of a variable to store
 * the result in and a source string. it returns NULL for success (and stores
 * a number in *v) or a static error message for failure.
 */
const char *wcalc(double *v, const char *s) {
	static Node nb[MAXMEM];
	ALLOC(Node) na = { nb, 0, COUNTOF(nb) };

	static double db[MAXMEM];
	ALLOC(double) da = { db, 0, COUNTOF(db) };

	static ppNode pb[MAXMEM];
	ALLOC(ppNode) pa = { pb, 0, COUNTOF(pb) };

	Node *root;
	const char *msg;

	if ((msg = parse(&root, s, &na, &da, &pa))) {
		return msg;
	}
	*v = eval(root);
	return NULL;
}

/* dear diary,
 * this is a simple I/O loop around the evaluator, mostly for testing
 * purposes.
 */
#ifdef WCALC_MAIN
int main(void) {
	char line[1230];

	while (fputs("> ", stdout), fflush(stdout), fgets(line, sizeof line, stdin)) {
		const char *msg;
		double v;
		/* fputs(line, stdout); */
		if ((msg = wcalc(&v, line))) {
			printf("error: %s\n", msg);
		} else {
			printf("%g\n", v);
		}
	}

	puts("exit");
	return 0;
}
#endif
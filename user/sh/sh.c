#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <lwk/liblwk.h>

#define MAXLINE 200		/* maximum line length */
#define WORD 256		/* token code for words */
#define EOF -1			/* token code for end of file */
#define ispunct(c)		(c=='|' || c=='&' || c==';' || c=='<' || \
				 c=='>' || c=='(' || c==')' || c=='\n')
#define isspace(c)		(c==' ' || c=='\t')
#define execute(np)		(ignored = (np? (*(np)->op)(np) : 0))

typedef struct Waitmsg Waitmsg;
struct Waitmsg{
	char *msg;
	int pid;
};

typedef struct Node	Node;
struct Node{			/* parse tree node */
	int (*op)(Node *); 	/* operator function */
	Node *args[2];		/* argument nodes */
	char *argv[100];	/* argument pointers */
	char *io[3];		/* i/o redirection */
};

Node	nodes[25];		/* node pool */
Node	*nfree;			/* next available node */
char	strspace[10*MAXLINE];	/* string storage */
char	*sfree;			/* next free character in strspace */
int	t;			/* current token code */
char 	*token;			/* current token text (in strspace) */
int	putback = 0;		/* lookahead */
char	status[256];		/* exit status of most recent command */
int	cflag = 0;		/* command is argument to sh */
int	tflag = 0;		/* read only one line */
int	interactive = 0;	/* prompt */
char	*cflagp;		/* command line for cflag */
char	*path[] ={"/bin", 0};
int	ignored;

Node	*alloc(int (*op)(Node *));
int	builtin(Node *np);
Node	*command(void);
int	getch(void);
int	gettoken(void);
Node	*list(void);
void	error(char *s, char *t);
Node	*pipeline(void);
void	redirect(Node *np);
int	setio(Node *np);
Node	*simple(void);
int	xpipeline(Node *np);
int	xsimple(Node *np);
int	xsubshell(Node *np);
int	xnowait(Node *np);
int	xwait(Node *np);
void exec(char *s, char *arg);

void ptab(int ntab)
{
	while(ntab--)
		printf("\t");
}

void dumpnode0(Node *np, int depth)
{
	int i;
	ptab(depth);
	if(np==0)
		return;
	printf("Node%d", depth);
	for(i = 0; i < 100; i++){
		if(np->argv[i]==0)
			break;
		printf(" %s", np->argv[i]);
	}
	printf("\n");
	dumpnode0(np->args[0], depth+1);
	dumpnode0(np->args[1], depth+1);
}
void
dumpnode(Node *np)
{
	dumpnode0(np, 0);
}

int
main(int argc, char *argv[])
{
	Node *np;

	printf("main argc %d %s\n", argc, argv+1);
	if(argc>1 && strcmp(argv[1], "-t")==0)
		tflag++;
	else if(argc>2 && strcmp(argv[1], "-c")==0){
		cflag++;
		cflagp = argv[2];
	}else if (argc>2 && strcmp(argv[1], "-f")==0){
		/* hack piggyback on cflag to open embedded script */
		cflag++;
		cflagp = strtoul(argv[2], NULL, 16);
	} else if(argc>1){
		close(0);
		if(open(argv[1], 0) != 0){
			error(": can't open", argv[1]);
			exit("argument");
		}
	}else
		interactive = 1;
	for(;;){
		if(interactive)
			fprintf(stderr, "%d$ ", getpid());
		nfree = nodes;
		sfree = strspace;
		if((t=gettoken()) == EOF)
			break;
		if(t != '\n')
			if((np = list()))
				execute(np);
			else
				error("syntax error", "");
		while(t!=EOF && t!='\n')	/* flush syntax errors */
			t = gettoken();
	}
	exit(status);
}

/* alloc - allocate for op and return a node */
Node*
alloc(int (*op)(Node *))
{
	printf("alloc\n");
	if(nfree < nodes+sizeof(nodes)){
		nfree->op = op;
		nfree->args[0] = nfree->args[1] = 0;
		nfree->argv[0] = nfree->argv[1] = 0;
		nfree->io[0] = nfree->io[1] = nfree->io[2] = 0;
		return nfree++;
	}
	error("node storage overflow", "");
	exit("node storage overflow");
	return NULL;
}

/* builtin - check np for builtin command and, if found, execute it */
int
builtin(Node *np)
{
	int n = 0;
	char name[MAXLINE];
	Waitmsg *wmsg;

	printf("builtin\n");
	dumpnode(np);

	if(np->argv[1])
		n = strtoul(np->argv[1], 0, 0);
	printf("%s %s n %d\n", np->argv[0], np->argv[1], n);
	if(strcmp(np->argv[0], "cd") == 0){
		if(chdir(np->argv[1]? np->argv[1] : "/") == -1)
			error(": bad directory", np->argv[0]);
		return 1;
	}else if(strcmp(np->argv[0], "exit") == 0)
		exit(np->argv[1]? np->argv[1] : status);
	else if(strcmp(np->argv[0], "ls") == 0){
		struct stat stats;
		DIR *dir;
		struct dirent *dentry;
		// what about pwd?
		if(np->argv[1]==0) {
			// no arguments mean we use the root for now.
			np->argv[1] = strdup("/");
//			error("usage: ls file", "");
		}
		if(stat(np->argv[1], &stats) < 0) {
			// TODO: use error here.
			printf("couldn't stat %s\n",  np->argv[1]);
			return 1;
		}
		if(S_ISDIR(stats.st_mode)) {
			dir = opendir(np->argv[1]);
			if(dir == NULL) {
				printf("couldn't open dir %s\n", np->argv[1]);
				return 1;
			}
			while((dentry = readdir(dir)) != NULL) {
				printf("%s\n", dentry->d_name);
			}
		}
		return 1;
	}else if(strcmp(np->argv[0], "echo") == 0){
		int i;
		int nflag;

		i=1;
		if(np->argv[1]!=0 && strcmp(np->argv[1], "-n")==0) {
			nflag++;
			i++;
		}
		for(; i < 100; i++) {
			if(np->argv[i]==0)
				break;
			printf("%s", np->argv[i]);
		}
		printf("\n");
		return 1;
	} else if(strcmp(np->argv[0], "cat") == 0){
		if(np->argv[1]==0 || np->argv[2]==0)
			error("usage: bind new old", "");
		else if(bind(np->argv[1], np->argv[2], 0)==-1)
			error("bind failed", "");
		return 1;
#ifdef asdf
	}else if(strcmp(np->argv[0], "unmount") == 0){
		if(np->argv[1] == 0)
			error("usage: unmount [new] old", "");
		else if(np->argv[2] == 0){
			if(unmount((char *)0, np->argv[1]) == -1)
				error("unmount:", "");
		}else if(unmount(np->argv[1], np->argv[2]) == -1)
			error("unmount", "");
		return 1;
#endif
	}else if(strcmp(np->argv[0], "wait") == 0){
		while((wmsg = wait()) != NULL){
			strncpy(status, wmsg->msg, sizeof(status)-1);
			if(n && wmsg->pid==n){
				n = 0;
				free(wmsg);
				break;
			}
			free(wmsg);
		}
		if(n)
			error("wait error", "");
		return 1;
	}else if(strcmp(np->argv[0], "rfork") == 0){
		// TODO: what is an rfork in kitten's work?

//		char *p;
//		int mask;
//
//		p = np->argv[1];
//		if(p == 0 || *p == 0)
//			p = "ens";
//		mask = 0;
//
//		while(*p)
//			switch(*p++){
//			case 'n': mask |= RFNAMEG; break;
//			case 'N': mask |= RFCNAMEG; break;
//			case 'e': mask |= RFENVG; break;
//			case 'E': mask |= RFCENVG; break;
//			case 's': mask |= RFNOTEG; break;
//			case 'f': mask |= RFFDG; break;
//			case 'F': mask |= RFCFDG; break;
//			case 'm': mask |= RFNOMNT; break;
//			default: error(np->argv[1], "bad rfork flag");
//			}
//		rfork(mask);

		return 1;
	}else if(strcmp(np->argv[0], "exec") == 0){
		redirect(np);
		if(np->argv[1] == (char *) 0)
			return 1;
		exec(np->argv[1], &np->argv[1]);
		n = np->argv[1][0];
		if(n!='/' && n!='#' && (n!='.' || np->argv[1][1]!='/'))
			for(n = 0; path[n]; n++){
				sprintf(name, "%s/%s", path[n], np->argv[1]);
				exec(name, &np->argv[1]);
			}
		error(": not found", np->argv[1]);
		return 1;
	}else if(strcmp(np->argv[0], "pa") == 0){
		struct pmem_region p;
		// XXX: add usage.
		// XXX: error check against bad pmem region.
		if(np->argv[1] == (char *) 0 || np->argv[2] == (char *) 0 || np->argv[3] == (char *) 0)
			return 1;
		if(strlen(np->argv[1]) > 32)
			fprintf(stderr, "ksh: warning physical memory region name longer than limit\n");
		strncpy(p.name,np->argv[1], 32);
		p.name_is_set = true;
		p.start = strtoul(np->argv[2], NULL, 16);

		p.end = strtoul(np->argv[3], NULL, 16);

		pmem_add(&p);

		return 1;
	}else if(strcmp(np->argv[0], "pu") == 0){
		struct pmem_region p;
		// XXX: add usage.
		// XXX: error check against bad pmem region.
		if(np->argv[1] == (char *) 0 || np->argv[2] == (char *) 0 || np->argv[3] == (char *) 0)
			return 1;
		if(strlen(np->argv[1]) > 32)
			fprintf(stderr, "ksh: warning physical memory region name longer than limit\n");
		strncpy(p.name,np->argv[1], 32);
		p.name_is_set = true;
		p.start = strtoul(np->argv[2], NULL, 16);

		p.end = strtoul(np->argv[3], NULL, 16);

		// do error handling?
		if(pmem_update(&p) < 0)
			return 0;

		return 1;
	}else if(strcmp(np->argv[0], "pq") == 0){
		struct pmem_region p0, p1;
		// XXX: add usage.
		// XXX: error check against bad pmem region.
		if(np->argv[1] == (char *) 0 || np->argv[2] == (char *) 0 || np->argv[3] == (char *) 0)
			return 1;

		if(strlen(np->argv[1]) > 32)
			fprintf(stderr, "ksh: warning physical memory region name longer than limit\n");
		strncpy(p0.name,np->argv[1], 32);
		p0.name_is_set = true;
		p0.start = strtoul(np->argv[2], NULL, 16);

		p0.end = strtoul(np->argv[3], NULL, 16);

// what does pmem query do for you?
		pmem_query(&p0, &p1);

		printf("%s 0x%x 0x%x\n", p1.name_is_set ? p1.name : "NO-NAME", p1.start, p1.end);
		return 1;
	}else if(strcmp(np->argv[0], "pa") == 0){
		size_t size1, size2;
		struct pmem_region p0, p1;

		if(np->argv[1] == (char *) 0 || np->argv[2] == (char *) 0 ||
				np->argv[3] == (char *) 0 && np->argv[3] == (char *) 0)
			return 1;

		p0.start = strtoul(np->argv[2], NULL, 16);
		p0.end = strtoul(np->argv[3], NULL, 16);

		pmem_alloc(size1, size2, &p0, &p1);
		return 1;
	}else if(strcmp(np->argv[0], "exec") == 0){
		redirect(np);
		if(np->argv[1] == (char *) 0)
			return 1;
		exec(np->argv[1], &np->argv[1]);
		n = np->argv[1][0];
		if(n!='/' && n!='#' && (n!='.' || np->argv[1][1]!='/'))
			for(n = 0; path[n]; n++){
				sprintf(name, "%s/%s", path[n], np->argv[1]);
				exec(name, &np->argv[1]);
			}
		error(": not found", np->argv[1]);
		return 1;
	}else if(strcmp(np->argv[0], "exec") == 0){
		redirect(np);
		if(np->argv[1] == (char *) 0)
			return 1;
		exec(np->argv[1], &np->argv[1]);
		n = np->argv[1][0];
		if(n!='/' && n!='#' && (n!='.' || np->argv[1][1]!='/'))
			for(n = 0; path[n]; n++){
				sprintf(name, "%s/%s", path[n], np->argv[1]);
				exec(name, &np->argv[1]);
			}
		error(": not found", np->argv[1]);
		return 1;
	}else if(strcmp(np->argv[0], "exec") == 0){
		redirect(np);
		if(np->argv[1] == (char *) 0)
			return 1;
		exec(np->argv[1], &np->argv[1]);
		n = np->argv[1][0];
		if(n!='/' && n!='#' && (n!='.' || np->argv[1][1]!='/'))
			for(n = 0; path[n]; n++){
				sprintf(name, "%s/%s", path[n], np->argv[1]);
				exec(name, &np->argv[1]);
			}
		error(": not found", np->argv[1]);
		return 1;
	}
	printf("success\n");
	return 0;
}

/* command - ( list ) [ ( < | > | >> ) word ]* | simple */
Node*
command(void)
{
	Node *np;

	printf("command\n");
	if(t != '(')
		return simple();
	np = alloc(xsubshell);
	t = gettoken();
	if((np->args[0]=list())==0 || t!=')')
		return 0;
	while((t=gettoken())=='<' || t=='>')
		if(!setio(np))
			return 0;
	return np;
}

/* getch - get next, possibly pushed back, input character */
int
getch(void)
{
	unsigned char c;
	static done=0;

	printf("getch\n");
	if(putback){
		c = putback;
		putback = 0;
	}else if(tflag){
		if(done || read(0, &c, 1)!=1){
			done = 1;
			return EOF;
		}
		if(c == '\n')
			done = 1;
	}else if(cflag){
		if(done)
			return EOF;
		if((c=*cflagp++) == 0){
			done = 1;
			c = '\n';
		}
	}else if(read(0, &c, 1) != 1)
		return EOF;
	printf("read ch %c\n", c);
	return c;
}

/* gettoken - get next token into string space, return token code */
int
gettoken(void)
{
	int c;

//	printf("gettoken\n");
	while((c = getch()) != EOF)
		if(!isspace(c))
			break;
	if(c==EOF || ispunct(c))
		return c;
	token = sfree;
	do{
		if(sfree >= strspace+sizeof(strspace) - 1){
			error("string storage overflow", "");
			exit("string storage overflow");
		}
		*sfree++ = c;
	}while((c=getch()) != EOF && !ispunct(c) && !isspace(c));
	*sfree++ = 0;
	putback = c;
	return WORD;
}

/* list - pipeline ( ( ; | & ) pipeline )* [ ; | & ]  (not LL(1), but ok) */
Node*
list(void)
{
	Node *np, *np1;

	printf("list\n");

	np = alloc(0);
	if((np->args[1]=pipeline()) == 0)
		return 0;
	while(t==';' || t=='&'){
		np->op = (t==';')? xwait : xnowait;
		t = gettoken();
		if(t==')' || t=='\n')	/* tests ~first(pipeline) */
			break;
		np1 = alloc(0);
		np1->args[0] = np;
		if((np1->args[1]=pipeline()) == 0)
			return 0;
		np = np1;
	}
	if(np->op == 0)
		np->op = xwait;
	return np;
}

/* error - print error message s, prefixed by t */
void
error(char *s, char *t)
{
	char buf[256];

	printf("error\n");
	fprintf(stderr, "%s%s", t, s);
//	errstr(buf, sizeof buf);
	fprintf(stderr, ": %s\n", buf);
}

/* pipeline - command ( | command )* */
Node*
pipeline(void)
{
	Node *np, *np1;

	printf("pipeline\n");
	if((np=command()) == 0)
		return 0;
	while(t == '|'){
		np1 = alloc(xpipeline);
		np1->args[0] = np;
		t = gettoken();
		if((np1->args[1]=command()) == 0)
			return 0;
		np = np1;
	}
	return np;
}

/* redirect - redirect i/o according to np->io[] values */
void
redirect(Node *np)
{
	int fd;

	printf("redirect\n");
	if(np->io[0]){
		if((fd = open(np->io[0], 0)) < 0){
			error(": can't open", np->io[0]);
			exit("open");
		}
		dup(fd, 0);
		close(fd);
	}
	if(np->io[1]){
		printf("tried to create()\n");
		for(;;);
//		if((fd = create(np->io[1], 1, 0666L)) < 0){
//			error(": can't create", np->io[1]);
//			exit("create");
//		}
		dup(fd, 1);
		close(fd);
	}
	if(np->io[2]){
		printf("tried to create() 2\n");
		for(;;);
//		if((fd = open(np->io[2], 1)) < 0 && (fd = create(np->io[2], 1, 0666L)) < 0){
//			error(": can't write", np->io[2]);
//			exit("write");
//		}
		dup(fd, 1);
		close(fd);
		printf("tried to seek\n");
		for(;;);
//		seek(1, 0, 2);
	}
}

/* setio - ( < | > | >> ) word; fill in np->io[] */
int
setio(Node *np)
{
	printf("setio\n");
	if(t == '<'){
		t = gettoken();
		np->io[0] = token;
	}else if(t == '>'){
		t = gettoken();
		if(t == '>'){
			t = gettoken();
			np->io[2] = token;
			}else
			np->io[1] = token;
	}else
		return 0;
	if(t != WORD)
		return 0;
	return 1;
}
			
/* simple - word ( [ < | > | >> ] word )* */
Node*
simple(void)
{
	Node *np;
	int n = 1;

	printf("simple\n");
	if(t != WORD)
		return 0;
	np = alloc(xsimple);
	np->argv[0] = token;
	while((t = gettoken())==WORD || t=='<' || t=='>')
		if(t == WORD)
			np->argv[n++] = token;
		else if(!setio(np))
			return 0;
	np->argv[n] = 0;
	return np;
}

/* xpipeline - execute cmd | cmd */
int
xpipeline(Node *np)
{
	int pid, fd[2];

	printf("xpipeline\n");
	if(pipe(fd) < 0){
		error("can't create pipe", "");
		return 0;
	}
	if((pid=fork()) == 0){	/* left side; redirect stdout */
		dup(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		execute(np->args[0]);
		exit(status);
	}else if(pid == -1){
		error("can't create process", "");
		return 0;
	}
	if((pid=fork()) == 0){	/* right side; redirect stdin */
		dup(fd[0], 0);
		close(fd[0]);
		close(fd[1]);
		pid = execute(np->args[1]); /*BUG: this is wrong sometimes*/
		if(pid > 0)
			while(waitpid()!=pid)
				;
		exit(0);
	}else if(pid == -1){
		error("can't create process", "");
		return 0;
	}
	close(fd[0]);	/* avoid using up fd's */
	close(fd[1]);
	return pid;
}

/* xsimple - execute a simple command */
int
xsimple(Node *np)
{
	char name[MAXLINE];
	int pid, i;

	printf("xsimple\n");
	if(builtin(np)){
		printf("returning\n");
		return 0;
	}
	printf("can't fork yet");
	for(;;);
//	if(pid = fork()){
//		if(pid == -1)
//			error(": can't create process", np->argv[0]);
//		return pid;
//	}
	redirect(np);	/* child process */
	exec(np->argv[0], &np->argv[0]);
	i = np->argv[0][0];
	if(i!='/' && i!='#' && (i!='.' || np->argv[0][1]!='/'))
		for(i = 0; path[i]; i++){
			sprintf(name, "%s/%s", path[i], np->argv[0]);
			exec(name, &np->argv[0]);
		}
	error(": not found", np->argv[0]);
	exit("not found");
	return -1;		// suppress compiler warnings
}

/* xsubshell - execute (cmd) */
int
xsubshell(Node *np)
{
	int pid;
	printf("can't fork yet");
	for(;;);
//	if(pid = fork()){
//		if(pid == -1)
//			error("can't create process", "");
//		return pid;
//	}
	redirect(np);	/* child process */
	execute(np->args[0]);
	exit(status);
	return -1;		// suppress compiler warnings
}

/* xnowait - execute cmd & */
int
xnowait(Node *np)
{
	int pid;

	printf("xnowait\n");
	execute(np->args[0]);
	pid = execute(np->args[1]);
	if(interactive)
		fprintf(stderr, "%d\n", pid);
	return 0;
}

/* xwait - execute cmd ; */
int xwait(Node *np)
{
	int pid;
	Waitmsg *wmsg;

	printf("xwait\n");
	execute(np->args[0]);
	pid = execute(np->args[1]);
	if(pid > 0){
		while((wmsg = wait()) != NULL){
			if(wmsg->pid == pid)
				break;
			free(wmsg);
		}
		if(wmsg == NULL)
			error("wait error", "");
		else {
			strncpy(status, wmsg->msg, sizeof(status)-1);
			free(wmsg);
		}
	}
	return 0;
}

void exec(char *s, char *arg)
{
	printf("exec\n");
}

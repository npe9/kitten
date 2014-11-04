
//extern Bench mpibench;
extern Bench tcasmbench;
extern Bench xpmembench;
extern Bench pipebench;
extern Bench filebench;
extern Bench posixqbench;
extern Bench kdbusbench;
extern Bench mmapbench;
extern Bench funcbench;
extern Bench basebench;
extern Bench arenabench;

Bench *benches[] = {
//		&mpibench,
		&tcasmbench,
		&xpmembench,
		&pipebench,
		&filebench,
		&posixqbench,
		&kdbusbench,
		&mmapbench,
		&funcbench,
		&basebench,
		&arenabench,
		NULL,
};

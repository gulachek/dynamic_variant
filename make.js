const { Command } = require('commander');
const { CppBuildCommand } = require('gulpachek/cpp');
const { spawn } = require('child_process');
const { series } = require('bach');

const cmd = new Command();
const cppBuild = new CppBuildCommand({
	program: cmd,
	cppVersion: 20
});

function makeLib(args) {
	const { cpp } = args;

	const dv = cpp.compile({
		name: 'com.gulachek.dynamic-variant',
		version: '0.1.0',
		apiDef: 'GULACHEK_DYNAMIC_VARIANT_API'
	});

	dv.include('include');

	const gtree = cpp.require('com.gulachek.gtree', '0.1.0');

	dv.link(gtree);

	return dv;
}

const test = cmd.command('test')
.description('unit tests');

cppBuild.configure(test, (args) => {
	const { cpp, sys } = args;

	const dv = makeLib(args);
	const boostTest = cpp.require('org.boost.unit-test-framework', '1.80.0', 'dynamic');

	const test = cpp.compile({
		name: 'dv_test',
		src: [ 'test/dynamic_variant_test.cpp' ]
	});

	test.link(dv);
	test.link(boostTest);

	const exe = test.executable();

	return series(sys.rule(exe), () => spawn(exe.abs(), { stdio: 'inherit' }));
});

cppBuild.pack(makeLib);

cmd.parse();

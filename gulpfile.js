const { task } = require('gulp');
const { spawn } = require('child_process');

const { BuildSystem, Target } = require('gulpachek');
const { Cpp } = require('gulpachek/cpp');

const { version } = require('./package.json');

if (!version) {
	console.error(new Error('dynamic_variant package.json version not specified'));
	process.exit(1);
}

const sys = new BuildSystem(__dirname);
const cpp = new Cpp(sys);

const boost = {
	test: cpp.require('org.boost.test', '1.68.0')
};

const librootPath = process.env.GULPACHEK_INSTALL_ROOT_CPPLIBROOT ||
	sys.dest('cpplibroot').abs();

class GtreeLib extends Target {
	constructor(sys) {
		super(sys);
	}

	build() {
		return spawn('npx', ['gulp', 'install'], {
			stdio: 'inherit',
			cwd: sys.src('gtree').abs(),
			env: { ...process.env,
				GULPACHEK_BUILD_DIR: sys.dest('gtree').abs(),
				GULPACHEK_INSTALL_ROOT_INCLUDE: sys.dest('include').abs(),
				GULPACHEK_INSTALL_ROOT_LIB: sys.dest('lib').abs(),
				GULPACHEK_INSTALL_ROOT_CPPLIBROOT: librootPath,
		}});
	}

	lib() {
		const origPath = process.env.CPP_LIBROOT_PATH;
		process.env.CPP_LIBROOT_PATH =
			`${librootPath}:${process.env.CPP_LIBROOT_PATH}`;

		const out = cpp.require('com.gulachek.gtree', '0.1.0');

		process.env.CPP_LIBROOT_PATH = origPath;
		return out;
	}
}

class DynamicVariant extends Target {
	#gtree;

	constructor(sys) {
		super(sys);
		this.#gtree = new GtreeLib(sys);
	}

	deps() {
		return [this.#gtree];
	}

	build(cb) {
		const test = cpp.executable('dynamic_variant_test',
			'test/dynamic_variant_test.cpp'
		);

		const lib = cpp.library('com.gulachek.dynamic-variant', version);
		lib.include('include');
		lib.link(this.#gtree.lib());

		test.link(lib);
		test.link(boost.test);
		return this.sys().rule(test)(cb);
	}
}

task('default', sys.rule(new DynamicVariant(sys)));

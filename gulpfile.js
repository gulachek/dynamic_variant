const { task } = require('gulp');
const { BuildSystem } = require('gulpachek');
const { Cpp } = require('gulpachek/cpp');
const dynamicVariant = require('./index.js');

const sys = new BuildSystem(__dirname);
const cpp = new Cpp(sys);

const lib = dynamicVariant.build(sys);

const boost = {
	test: cpp.require('org.boost.test', '1.68.0')
};

const test = cpp.executable('dynamic_variant_test',
	'test/dynamic_variant_test.cpp'
);

test.link(lib);
test.link(boost.test);

task('default', sys.rule(test));

const { Cpp } = require('gulpachek/cpp');
const { version } = require('./package.json');
const gtree = require('./gtree');

if (!version) {
	console.error(new Error('dynamic_variant package.json version not specified'));
	process.exit(1);
}

function build(sys) {
	const cpp = new Cpp(sys);

	const gtreeLib = gtree.build(sys.sub('gtree'));

	const lib = cpp.library('com.gulachek.dynamic-variant', version);
	lib.include('include');
	lib.link(gtreeLib);

	return lib;
}

module.exports = {
	build
};

const Builder = @import("std").build.Builder;

pub fn build(b: *Builder) void {
    const mode = b.standardReleaseOptions();

    const lib_root_src: []const u8 = "src/lib.zig";

    const lib = b.addStaticLibrary("zig-regex", lib_root_src);
    lib.setBuildMode(mode);
    lib.install();

    var main_tests = b.addTest(lib_root_src);
    main_tests.setBuildMode(mode);

    const test_step = b.step("test", "Run library tests");
    test_step.dependOn(&main_tests.step);
}

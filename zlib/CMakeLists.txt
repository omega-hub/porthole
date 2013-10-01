add_definitions(-DFASTEST)
set(zlib_SOURCES
         ./gzlib.c
		 ./adler32.c
		 ./compress.c
		 ./crc32.c
		 ./deflate.c
		 ./gzclose.c
		 ./gzio.c
		 ./gzread.c
		 ./gzwrite.c
		 ./infback.c
		 ./inffast.c
		 ./inflate.c
		 ./inftrees.c
		 ./trees.c
		 ./uncompr.c
		 ./zutil.c
)

add_library(zlib STATIC ${zlib_SOURCES})
SET_TARGET_PROPERTIES(zlib PROPERTIES FOLDER 3rdparty)

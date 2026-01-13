The Perforce check in structure for the Sony GNMX source is a bit confussing, so here is the explanation.

"//3rdParty/dev/libs/gnmx/unmodified/" contains unmodified Sony source code only.

"//3rdParty/dev/libs/gnmx/modified/" contains modifications such as include paths, etc.

The reason for these two "//3rdParty/dev/libs/gnmx/(un)modified/" subdirectories is to keep the Perforce history cleaner.  When updating to a new SDK, the source code can be splatted over "//3rdParty/dev/libs/gnmx/unmodified/", then integrated across to "//3rdParty/dev/libs/gnmx/modified/".  Perforce then has the correct history information to redo any local changes easily during the merge.

Finally, the code is then integrated from "//3rdParty/dev/libs/gnmx/modified/" to something like "//rage/gta5/dev_ng/rage/base/src/grcore/gnmx/1.6/".  Wrapper files in "//rage/gta5/dev_ng/rage/base/src/grcore/gnmx/" then include the corresponding file from the correct subdirectory based on the current SDK version.  This allows us to support multiple SDK versions simultaneously in the code base.

If any changes are required to be made to the GNMX source, make them first to "//3rdParty/dev/libs/gnmx/modified/", then integrate over the game.

Updating all the wrapper files when there is a new SDK is a bit annoying, something along the lines of

    for f in `l1` ; do \
        if [ -f "$f" ] ; then \
            echo "$f"; \
            dos2unix "$f"; \
            cat "$f" | sed 's%^#else%#elif (SCE_ORBIS_SDK_VERSION \& 0xfff00000) == 0x01700000\n#\tinclude "1.7/'"$f"'"\n#else%' > /tmp/foo; \
            mv /tmp/foo "$f"; \
            unix2dos "$f"; \
            echo; \
        fi; \
    done

can patch in the SDK include to the wrapper files.  Be careful of newly added or removed files though.

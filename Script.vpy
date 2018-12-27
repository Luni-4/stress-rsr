import vapoursynth as vs
core = vs.core

# Open a file
path = 'path/to/file'
video = core.ffms2.Source(source = path)

# Convert to RGB 24 bit
video = core.fmtc.resample (clip=video, css="444")
video = core.fmtc.matrix (clip=video, mat="601", col_fam=vs.RGB)
video = core.fmtc.bitdepth (clip=video, bits=8)

# Run stress
video = core.st.Stress(
    video,
    iterations= 10,
    samples = 10,
    ray=1,
    algorithm="Stress"
)

video.set_output()

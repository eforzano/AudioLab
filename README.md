# AudioLab
A JUCE plugin to test audio effects. 

The source of the audio can be:
- Use the audio from your computer or audio interface.
- Use a sound file that can be looped
- a waveform type:
    - sine
    - square
    - triangle
    - several noise types


# Build 

## MacOS / Linux
```
cmake -B build
cmake --build build --config Release
```

## Windows
```
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

## JUCE Location
If JUCE isn't a sibling folder, point to it explicitly:
bashcmake -B build -DJUCE_PATH=/path/to/JUCE

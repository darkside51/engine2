#!/usr/bin/env python3
# use example linux:   $ python3 shaders_convert.py -i ./glsl/ -o ./spirv/ -p /bin/glslc
# use example windows: $ python shaders_convert.py -i ./glsl/ -o ./spirv/ -p /Bin/glslc.exe

import sys
import getopt
import os
import shutil
import subprocess


def convert(inDir, fileName, outDir, glslcPath):
    fileData = ""
    with open(os.path.join(inDir, fileName), 'r', encoding='utf-8') as file:
        for line in file:
            fileData = fileData + line
            # if line.find('#import') == 0:
            #     includeFileName = line.replace('#import ', '')
            #     includeFileName = includeFileName.replace('\n', '')
            #     file2 = open(inDir + includeFileName, 'r')
            #     fileData2 = file2.read()
            #     fileData = fileData + fileData2
            #     file2.close()
            # else:
            #     fileData = fileData + line

    tmpFileName = os.path.join(inDir, 'tmp', fileName.replace('.glsl', ''))
    with open(tmpFileName, 'w', encoding='utf-8') as file3:
        file3.write(fileData)

    outFile = os.path.join(outDir, fileName.replace('.glsl', '.spv'))
    script_dir = os.path.dirname(os.path.abspath(__file__))
    include_dir = os.path.normpath(os.path.join(script_dir, '..', '..', 'src', 'shaders', 'glsl'))

    # glslcPath может быть как именем из PATH, так и полным путём (Vulkan SDK/Bin/glslc.exe).
    result = subprocess.run([glslcPath, '-I', include_dir, '-I', inDir, '-O', tmpFileName, '-o', outFile])
    if result.returncode != 0:
        raise RuntimeError('glslc failed for %s (exit code %d)' % (fileName, result.returncode))

    os.remove(tmpFileName)


def checkIsShaderFile(fileName):
    return fileName.endswith(".glsl")


# entry point
if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], "i:o:p:", ["ifolder=", "ofolder=", "path="])
    except getopt.GetoptError:
        sys.exit(1)

    # check parameters length
    if len(opts) < 1:
        sys.exit(2)

    # parse parameters
    inputFolder = ''
    outputFolder = ''
    glslc_path = 'glslc'
    for opt, arg in opts:
        if opt in ("-i", "--ifolder"):
            inputFolder = arg
        elif opt in ("-o", "--ofolder"):
            outputFolder = arg
        elif opt in ("-p", "--path"):
            glslc_path = arg

    # print(f'#### [shader converter] glslc path: {glslc_path}', flush=True)

    if os.path.exists(inputFolder):
        if not os.path.exists(outputFolder):
            os.makedirs(outputFolder, exist_ok=True)

        tmpDir = os.path.join(inputFolder, 'tmp')
        if not os.path.exists(tmpDir):
            os.makedirs(tmpDir, exist_ok=True)

        for path in os.listdir(inputFolder):
            # check if current path is a file
            if os.path.isfile(os.path.join(inputFolder, path)) and checkIsShaderFile(path):
                convert(inputFolder, path, outputFolder, glslc_path)

        shutil.rmtree(tmpDir, ignore_errors=True)
        print('*glsl->spirv conversion complete*')

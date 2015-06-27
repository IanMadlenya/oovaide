// oovCMaker.h
// Created on: Jan 23, 2014
// \copyright 2014 DCBlaha.  Distributed under the GPL.
#include "Version.h"
#include "Project.h"
#include "oovCMaker.h"
#include "OovString.h"
#include <ctype.h>
#include <algorithm>
#include <stdio.h>



static bool compareNoCase(const std::string& first, const std::string& second)
    {
    OovString str1;
    OovString str2;
    str1.setLowerCase(first);
    str2.setLowerCase(second);
    return(str1 < str2);
    }

void CMaker::writeFile(OovStringRef const destName, OovStringRef const str)
    {
    if(str.numBytes() > 0)
        {
        FILE *fp = fopen(destName, "w");
        if(fp)
            {
            fprintf(fp, "%s", "# Generated by oovCMaker\n");
            fprintf(fp, "%s", str.getStr());
            fclose(fp);
            }
        else
            {
            fprintf(stderr, "Unable to open file %s\n", destName.getStr());
            }
        }
    }

void CMaker::makeDefineName(OovStringRef const pkgName, OovString &defName)
    {
    OovString shortName;
    for(const char *p = pkgName; *p!='\0'; p++)
        {
        if(isalpha(*p))
            shortName += *p;
        }
    defName.setUpperCase(shortName);
    }

static std::string makeComponentNameFromDir(std::string const &str)
    {
    std::string fixedCompName = str;
    std::replace(fixedCompName.begin(), fixedCompName.end(), '/', '-');
    return fixedCompName;
    }

void CMaker::addPackageDefines(OovStringRef const pkgName, std::string &str)
    {
    OovString pkgDefName;
    makeDefineName(pkgName, pkgDefName);
    str += std::string("# ") + pkgDefName + "\n";
    str += std::string("pkg_check_modules(") + pkgDefName + " REQUIRED " + pkgName.getStr() + ")\n";
    str += std::string("include_directories(${") + pkgDefName + "_INCLUDE_DIRS})\n";
    str += std::string("link_directories(${") + pkgDefName + "_LIBRARY_DIRS})\n";
    str += std::string("add_definitions(${") + pkgDefName + "_CFLAGS_OTHER})\n";
    }

void CMaker::makeTopMakelistsFile(OovStringRef const destName)
    {
    std::string str;

    str += "cmake_minimum_required(VERSION 2.8)\n";
    str += std::string("project(") + mProjectName + ")\n";
    str += "INCLUDE(FindPkgConfig)\n\n";

    OovString projNameUpper;
    projNameUpper.setUpperCase(mProjectName);
    str += std::string("set(") + projNameUpper + "_MAJOR_VERION 0)\n";
    str += std::string("set(") + projNameUpper + "_MINOR_VERION 1)\n";
    str += std::string("set(") + projNameUpper + "_PATCH_VERION 0)\n";
    str += std::string("set(") + projNameUpper + "_VERION\n";
    str += std::string("${") + projNameUpper + "_MAJOR_VERSION}.${";
    str += projNameUpper + "_MINOR_VERSION}.${";
    str += projNameUpper + "_PATCH_VERSION})\n\n";

    str += "# Offer the user the choice of overriding the installation directories\n";
    str += "set(INSTALL_LIB_DIR lib CACHE PATH \"Installation directory for libraries\")\n";
    str += "set(INSTALL_BIN_DIR bin CACHE PATH \"Installation directory for executables\")\n";
    str += "set(INSTALL_INCLUDE_DIR include CACHE PATH\n";
    str += "   \"Installation directory for header files\")\n";
    str += "if(WIN32 AND NOT CYGWIN)\n";
    str += "   set(DEF_INSTALL_CMAKE_DIR CMake)\n";
    str += "else()\n";
    str += std::string("   set(DEF_INSTALL_CMAKE_DIR lib/CMake/") + mProjectName + ")\n";
    str += "endif()\n";
    str += "set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH\n";
    str += "   \"Installation directory for CMake files\")\n\n";

    str += "# Make relative paths absolute (needed later on)\n";
    str += "foreach(p LIB BIN INCLUDE CMAKE)\n";
    str += "   set(var INSTALL_${p}_DIR)\n";
    str += "   if(NOT IS_ABSOLUTE \"${${var}}\")\n";
    str += "      set(${var} \"${CMAKE_INSTALL_PREFIX}/${${var}}\")\n";
    str += "   endif()\n";
    str += "endforeach()\n\n";

    str += "# Add debug and release flags\n";
    str += "# Use from the command line with -DCMAKE_BUILD_TYPE=Release or Debug\n";
    str += "set(CMAKE_CXX_FLAGS_DEBUG \"${CMAKE_CXX_FLAGS_DEBUG} -Wall\")\n";
    str += "set(CMAKE_CXX_FLAGS_RELEASE \"${CMAKE_CXX_FLAGS_RELEASE} -Wall\")\n";

    str += "# External Packages\n";
    str += "if(NOT WIN32)\n";
    for(auto const &pkg : mBuildPkgs.getPackages())
        {
        addPackageDefines(pkg.getPkgName(), str);
        }
    str += "endif()\n\n";

    str += "# set up include directories\n";
    str += "include_directories(\n";

    OovStringVec const compNames = mCompTypes.getComponentNames(true);
    for(auto const &name : compNames)
        {
        ComponentTypesFile::eCompTypes compType = mCompTypes.getComponentType(name);
        if(compType == ComponentTypesFile::CT_StaticLib)
            str += std::string("   \"${PROJECT_SOURCE_DIR}/") + name + "\"\n";
        }
    str += "   )\n";
    str += "add_definitions(-std=c++11)\n\n";

    str += "# Add sub directories\n";
    for(auto const &name : compNames)
        {
        ComponentTypesFile::eCompTypes compType = mCompTypes.getComponentType(name);
        if(compType != ComponentTypesFile::CT_Unknown)
            {
            std::string compRelDir = name;
            str += std::string("add_subdirectory(") + compRelDir + ")\n";
            }
        }

    str += "# Add all targets to the build-tree export set\n";
    str += "export(TARGETS ";
    for(auto const &name : compNames)
        {
        ComponentTypesFile::eCompTypes compType = mCompTypes.getComponentType(name);
        if(compType != ComponentTypesFile::CT_Unknown)
            {
            str += ' ';
            str += makeComponentNameFromDir(name);
            }
        }
    str += "\n";
    str += "   FILE \"${PROJECT_BINARY_DIR}/" + mProjectName + "Targets.cmake\")\n\n";

    str += "# Export the package for use from the build-tree\n";
    str += "# (this registers the build-tree with a global CMake-registry)\n";
    str += std::string("export(PACKAGE ") + mProjectName + ")\n\n";

    str += std::string("# Create the ") +  mProjectName + "Config.cmake and " +
            mProjectName + "ConfigVersion files\n";
    str += "file(RELATIVE_PATH REL_INCLUDE_DIR \"${INSTALL_CMAKE_DIR}\"\n";
    str += "   \"${INSTALL_INCLUDE_DIR}\")\n";
    str += "# for the build tree\n";
    str += "set(CONF_INCLUDE_DIRS \"${PROJECT_SOURCE_DIR}\" \"${PROJECT_BINARY_DIR}\")\n";
    str += std::string("configure_file(") + mProjectName + "Config.cmake.in\n";
    str += std::string("   \"${PROJECT_BINARY_DIR}/") + mProjectName + "Config.cmake\" @ONLY)\n";
    str += "# for the install tree\n";
    str += "set(CONF_INCLUDE_DIRS \"\\${" + projNameUpper + "_CMAKE_DIR}/${REL_INCLUDE_DIR}\")\n";
    str += std::string("configure_file(") + mProjectName + "Config.cmake.in\n";
    str += std::string("   \"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/") +
            mProjectName + "Config.cmake\" @ONLY)\n";
    str += "# for both\n";
    str += std::string("configure_file(") + mProjectName + "ConfigVersion.cmake.in\n";
    str += std::string("   \"${PROJECT_BINARY_DIR}/") + mProjectName +
            "ConfigVersion.cmake\" @ONLY)\n\n";

    str += std::string("# Install the ") + mProjectName + "Config.cmake and " +
            mProjectName + "ConfigVersion.cmake\n";
    str += "install(FILES\n";
    str += std::string("   \"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/") +
            mProjectName + "Config.cmake\"\n";
    str += std::string("   \"${PROJECT_BINARY_DIR}/") + mProjectName +
            "ConfigVersion.cmake\"\n";
    str += "DESTINATION \"${INSTALL_CMAKE_DIR}\" COMPONENT dev)\n\n";

    str += "# Install the export set for use with the install-tree\n";
    str += std::string("install(EXPORT ") + mProjectName + "Targets DESTINATION\n";
    str += "   \"${INSTALL_CMAKE_DIR}\" COMPONENT dev)\n";
    writeFile(destName, str);
    }

void CMaker::makeTopInFile(OovStringRef const destName)
    {
    OovString projNameUpper;
    projNameUpper.setUpperCase(mProjectName);
    OovStringVec const compNames = mCompTypes.getComponentNames(true);

    std::string str = std::string("# - Config file for the ") + mProjectName + " package\n";

    str += "# Compute paths\n";
    str += std::string("get_filename_component(") + projNameUpper +
            "_CMAKE_DIR \"${CMAKE_CURRENT_LIST_FILE}\" PATH)\n";
    str += std::string("set(") + projNameUpper + "_INCLUDE_DIRS \"@CONF_INCLUDE_DIRS@\")\n\n";

    str += "# Our library dependencies (contains definitions for IMPORTED targets)\n";
    str += std::string("if(NOT TARGET ") + mProjectName + "AND NOT " +
            projNameUpper + "_BINARY_DIR)\n";
    str += "   include(\"${FOOBAR_CMAKE_DIR}/" + mProjectName + "Targets.cmake\")\n";
    str += "endif()\n\n";

    str += std::string("# These are IMPORTED targets created by ") + mProjectName +
            "Targets.cmake\n";
    str += std::string("set(") + projNameUpper + "_LIBRARIES";
    for(auto const &name : compNames)
        {
        ComponentTypesFile::eCompTypes compType = mCompTypes.getComponentType(name);
        if(compType == ComponentTypesFile::CT_StaticLib)
            {
            str += ' ';
            str += name;
            }
        }
    str += ")\n";
    str += std::string("set(") + projNameUpper + "_EXECUTABLE";
    for(auto const &name : compNames)
        {
        ComponentTypesFile::eCompTypes compType = mCompTypes.getComponentType(name);
        if(compType == ComponentTypesFile::CT_Program)
            {
            str += ' ';
            str += name;
            }
        }
    str += ")\n";
    writeFile(destName, str);
    }

void CMaker::makeTopVerInFile(OovStringRef const destName)
    {
    OovString projNameUpper;
    projNameUpper.setUpperCase(mProjectName);

    std::string str;
    str += "set(PACKAGE_VERSION \"@" + projNameUpper + "_VERSION@\")\n\n";

    str += "# Check whether the requested PACKAGE_FIND_VERSION is compatible\n";
    str += "if(\"${PACKAGE_VERSION}\" VERSION_LESS \"${PACKAGE_FIND_VERSION}\")\n";
    str += "   set(PACKAGE_VERSION_COMPATIBLE FALSE)\n";
    str += "else()\n";
    str += "   set(PACKAGE_VERSION_COMPATIBLE TRUE)\n";
    str += "   if (\"${PACKAGE_VERSION}\" VERSION_EQUAL \"${PACKAGE_FIND_VERSION}\")\n";
    str += "      set(PACKAGE_VERSION_EXACT TRUE)\n";
    str += "   endif()\n";
    str += "endif()\n";
    writeFile(destName, str);
    }

void CMaker::makeToolchainFile(OovStringRef const compilePath,
        OovStringRef const destFileName)
    {
    std::string str;
    str += "SET(CMAKE_SYSTEM_NAME Linux)\n";
    str += "SET(CMAKE_SYSTEM_PROCESSOR \"arm\")\n";
    str += "include(CMakeForceCompiler)\n";
//    str += "CMAKE_FORCE_C_COMPILER(arm-linux-gnueabihf-gcc GNU)\n";
    str += "CMAKE_FORCE_CXX_COMPILER(";
    str += compilePath;
    str += " GNU)\n";

    writeFile(destFileName, str);
    }


void CMaker::appendNames(OovStringVec const &names,
        char delim, std::string &str)
    {
    for(auto const &name : names)
        {
        str += name;
        str += delim;
        size_t startpos = str.rfind('\n');
        if(startpos == std::string::npos)
            startpos = 0;
        if(str.length() - startpos > 70)
            {
            str += "\n  ";
            }
        }
    int len = str.length();
    if(len > 0 && str[len-1] == delim)
        str.resize(len-1);
    }

enum eCommandTypes { CT_Exec, CT_Shared, CT_Static,
    CT_Interface, // A library with only headers
    CT_TargHeaders, CT_TargLinkLibs };

static void appendCommandAndNames(eCommandTypes ct, char const *compName,
        OovStringVec const &names, OovString &str)
    {
    switch(ct)
        {
        case CT_Shared:
        case CT_Static:
        case CT_Interface:
            str += "add_library";
            break;

        case CT_Exec:           str += "add_executable";        break;
        case CT_TargHeaders:    str += "set";                   break;
        case CT_TargLinkLibs:   str += "target_link_libraries"; break;
        }
    str += "(";
    if(ct == CT_TargHeaders)
        {
        str += "HEADER_FILES ";
        }
    else
        {
        str += compName;
        }
    switch(ct)
        {
        case CT_Exec:           str += ' ';             break;
        case CT_Shared:         str += " SHARED ";      break;
        case CT_Static:         str += " STATIC ";      break;
        // Using INTERFACE at this time gives:
        // "Cannot find source file:", since CMake doesn't understand interface keyword.
//      case CT_Interface:      str += " INTERFACE ";   break;
        case CT_Interface:      str += " STATIC ";      break;
        case CT_TargHeaders:    str += ' ';             break;
        case CT_TargLinkLibs:   str += ' ';             break;
        }
    CMaker::appendNames(names, ' ', str);
    str += ")\n\n";
    if(ct == CT_TargHeaders)
        {
        str += "set_target_properties(";
        str += compName;
        str += " PROPERTIES PUBLIC_HEADER \"${HEADER_FILES}\")\n\n";
        }
    }

void CMaker::makeComponentFile(OovStringRef const compName,
    ComponentTypesFile::eCompTypes compType,
    OovStringVec const &sources, OovStringRef const destName)
    {
    OovString str;
    if(mVerbose)
        printf("Processing %s\n      %s\n", compName.getStr(), destName.getStr());
    if(compType == ComponentTypesFile::CT_Program)
        {
        if(mVerbose)
            printf("  Executable\n");

        appendCommandAndNames(CT_Exec, compName, sources, str);

        OovStringVec libs = getCompLibraries(compName);
        appendCommandAndNames(CT_TargLinkLibs, compName, libs, str);

        str += "install(TARGETS ";
        str += compName;
        str += "\n  EXPORT ";
        str += mProjectName;
        str += "Targets";
        str += "\n  RUNTIME DESTINATION \"${INSTALL_BIN_DIR}\" COMPONENT lib)\n";
        }
    else if(compType == ComponentTypesFile::CT_SharedLib)
        {
        if(mVerbose)
            printf("  SharedLib\n");
        appendCommandAndNames(CT_Shared, compName, sources, str);

        OovStringVec libs = getCompLibraries(compName);
        appendCommandAndNames(CT_TargLinkLibs, compName, libs, str);

        str += "install(TARGETS ";
        str += compName;
        str += "\n  LIBRARY DESTINATION \"${INSTALL_LIB_DIR}\" COMPONENT lib)\n";
        }
    else if(compType == ComponentTypesFile::CT_StaticLib)
        {
        if(mVerbose)
            printf("  Library\n");
        OovStringVec headers = mCompTypes.getComponentIncludes(compName);
        discardDirs(headers);

        OovStringVec allFiles = headers;
        allFiles.insert(allFiles.end(), sources.begin(), sources.end() );
        std::sort(allFiles.begin(), allFiles.end(), compareNoCase);
        if(sources.size() == 0)
            {
            appendCommandAndNames(CT_Interface, compName, allFiles, str);
            str += "set_target_properties(";
            str += compName;
            str += " PROPERTIES LINKER_LANGUAGE CXX)\n";
            }
        else
            appendCommandAndNames(CT_Static, compName, allFiles, str);

        appendCommandAndNames(CT_TargHeaders, compName, headers, str);


        str += "install(TARGETS ";
        str += compName;
        str += "\n  EXPORT ";
        str += mProjectName;
        str += "Targets";
        str += "\n  ARCHIVE DESTINATION \"${INSTALL_LIB_DIR}\" COMPONENT lib";
        str += "\n  PUBLIC_HEADER DESTINATION \"${INSTALL_INCLUDE_DIR}/";
        str += mProjectName;
        str += "\" COMPONENT dev)\n";
        }
    writeFile(destName, str);
    }

OovStringVec CMaker::getCompSources(OovStringRef const compName)
    {
    OovStringVec sources = mCompTypes.getComponentSources(compName);
    OovString compPath = mCompTypes.getComponentAbsolutePath(compName);
    for(auto &src : sources)
        {
        src.erase(0, compPath.length());
        }
    std::sort(sources.begin(), sources.end(), compareNoCase);
    return sources;
    }

OovStringVec CMaker::getCompLibraries(OovStringRef const compName)
    {
    OovStringVec libs;
/*
    OovStringVec projectLibFileNames;
    mObjSymbols.appendOrderedLibFileNames("ProjLibs", getSymbolBasePath(),
            projectLibFileNames);
*/
    /// @todo - this does not order the libraries.
    OovStringVec srcFiles = mCompTypes.getComponentSources(compName);
    OovStringSet projLibs;
    OovStringVec compNames = mCompTypes.getComponentNames(true);
    for(auto const &srcFile : srcFiles)
        {
        FilePath fp;
        fp.getAbsolutePath(srcFile, FP_File);
        OovStringVec incDirs =
            mIncMap.getNestedIncludeDirsUsedBySourceFile(fp);
        for(auto const &supplierCompName : compNames)
            {
            if(mCompTypes.getComponentType(supplierCompName) ==
                    ComponentTypesFile::CT_StaticLib)
                {
                std::string compDir = mCompTypes.getComponentAbsolutePath(
                        supplierCompName);
                for(auto const &incDir : incDirs)
                    {
                    if(compDir.compare(incDir) == 0)
                        {
                        if(supplierCompName.compare(compName) != 0)
                            {
                            projLibs.insert(makeComponentNameFromDir(
                                    supplierCompName));
                            break;
                            }
                        }
                    }
                }
            }
        }
    std::copy(projLibs.begin(), projLibs.end(), std::back_inserter(libs));

    for(auto const &pkg : mBuildPkgs.getPackages())
        {
        OovString compDir = mCompTypes.getComponentAbsolutePath(compName);
        OovStringVec incRoots = pkg.getIncludeDirs();
        if(mIncMap.anyRootDirsMatch(incRoots, compDir))
            {
            if(pkg.getPkgName().compare(compName) != 0)
                {
                OovString pkgRef = "${";
                OovString pkgDefName;
                makeDefineName(pkg.getPkgName(), pkgDefName);
                pkgRef += pkgDefName + "_LIBRARIES}";
                libs.push_back(pkgRef);
                }
            }
        }
    return libs;
    }

void CMaker::makeTopLevelFiles(OovStringRef const outDir)
    {
    FilePath topVerInFp(outDir, FP_File);
    topVerInFp.appendFile(std::string(mProjectName + "ConfigVersion.cmake.in"));
    makeTopVerInFile(topVerInFp);

    FilePath topInFp(outDir, FP_File);
    topInFp.appendFile(std::string(mProjectName + "Config.cmake.in"));
    makeTopInFile(topInFp);
    }

void CMaker::makeToolchainFiles(OovStringRef const outDir)
    {
    if(mBuildOptions.read())
        {
        std::string buildConfigStr = mBuildOptions.getValue(OptBuildConfigs);
        if(buildConfigStr.length() > 0)
            {
            CompoundValue buildConfigs(buildConfigStr);
            for(auto const &config : buildConfigs)
                {
                OovString optStr = makeBuildConfigArgName(OptToolCompilePath, config);
                OovString compilePath = mBuildOptions.getValue(optStr);

                FilePath outFp(outDir, FP_Dir);
                std::string fn = config + ".cmake";
                outFp.appendFile(fn);
                makeToolchainFile(compilePath, outFp);
                }
            }
        }
    }

// outDir ignored if writeToProject is true.
void CMaker::makeComponentFiles(bool writeToProject, OovStringRef const outDir,
        OovStringVec const &compNames)
    {
    FilePath incMapFn(getAnalysisPath(), FP_Dir);
    incMapFn.appendFile(Project::getAnalysisIncDepsFilename());
    mIncMap.read(incMapFn);
    if(mVerbose)
        printf("Read incmap\n");
    for(auto const &compName : compNames)
        {
        ComponentTypesFile::eCompTypes compType =
            mCompTypes.getComponentType(compName);
        if(compType != ComponentTypesFile::CT_Unknown)
            {
            OovStringVec sources = getCompSources(compName);
            FilePath outFp;
            std::string fixedCompName = makeComponentNameFromDir(compName);
            if(writeToProject)
                {
                outFp.setPath(mCompTypes.getComponentAbsolutePath(
                        compName), FP_Dir);
                outFp.appendFile("CMakeLists.txt");
                }
            else
                {
                outFp.setPath(outDir, FP_File);
                outFp.appendFile(std::string(fixedCompName + "-CMakeLists.txt"));
                }
            // Using the filepath here gives:
            // "Error evaluating generator expression", and "Target name not supported"
            makeComponentFile(fixedCompName, compType,
                    sources, outFp);
            }
        }
    }

/*
OovString CMaker::getComponentAbsolutePath(OovStringRef const compName)
    {
    auto const mapIter = mCachedComponentPaths.find(compName);
    if(mapIter == mCachedComponentPaths.end())
        {
        mCachedComponentPaths[compName] = mCompTypes.getComponentAbsolutePath(compName);
        }
    return mCachedComponentPaths[compName];
    }
*/

int main(int argc, char * argv[])
    {
    int ret = 1;
    bool verbose = false;
    bool writeToProject = false;
    const char *projDir = nullptr;
    const char *projName = "PROJNAME";

    for(int argi=1; argi<argc; argi++)
        {
        if(argv[argi][0] == '-')
            {
            switch(argv[argi][1])
                {
                case 'v':
                    verbose = true;
                    break;

                case 'w':
                    writeToProject = true;
                    break;

                case 'n':
                    projName = &argv[argi][2];
                    break;
                }
            }
        else
            projDir = argv[argi];
        }
    if(projDir)
        {
        Project::setProjectDirectory(projDir);
        CMaker maker(projName, verbose);

        FilePath outDir;
        if(writeToProject)
            {
            outDir.setPath(Project::getSrcRootDirectory(), FP_Dir);
            }
        else
            {
            outDir.setPath(Project::getOutputDir("CMake"), FP_Dir);
            FileEnsurePathExists(outDir);
            if(maker.mVerbose)
                printf("Output directory %s\n", outDir.getStr());
            }

        if(maker.mCompTypes.read())
            {
            maker.makeToolchainFiles(outDir);
            maker.makeTopLevelFiles(outDir);

            maker.mBuildPkgs.read();

            FilePath topMlFp(outDir, FP_File);
            topMlFp.appendFile("CMakeLists.txt");
            maker.makeTopMakelistsFile(topMlFp);

            OovStringVec compNames = maker.mCompTypes.getComponentNames(true);
            if(compNames.size() > 0)
                {
                maker.makeComponentFiles(writeToProject, outDir, compNames);
                ret = 0;
                }
            else
                fprintf(stderr, "Components must be defined\n");
            }
        else
            fprintf(stderr, "Unable to read project files\n");
        }
    else
        {
        fprintf(stderr, "OovCMaker version %s\n", OOV_VERSION);
        fprintf(stderr, "  OovCMaker projectDirectory [switches]\n");
        fprintf(stderr, "    switches -v=verbose, -w=write to project, -nProjName\n");
        }
    return ret;
    }

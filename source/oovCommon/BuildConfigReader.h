/*
 * BuildConfigReader.h
 *
 *  Created on: Jan 13, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef BUILDCONFIGREADER_H_
#define BUILDCONFIGREADER_H_

#include "NameValueFile.h"
#include "Project.h"

// The BuildConfig file stores all arguments for each build configuration such
// as analysis, debug, or release. It does this in a format that allows
// distinguishing which arguments have changed to cause the least amount of
// rescanning or rebuilding.  It saves a CRC for different types of arguments
// to allow simple comparison to check if arguments have changed.
//
// Format of BuildConfig file:
//      ana|<AnalysisArgsCrc>;<ExtPathArgsCrc>;<ExtLinkPathArgsCrc>;<ProjPathArgsCrc>;<LinkArgsCrc>;<OtherArgsCrc>
//      dbg|<AnalysisArgsCrc>;<ExtPathArgsCrc>;<ExtLinkPathArgsCrc>;<ProjPathArgsCrc>;<LinkArgsCrc>;<OtherArgsCrc>
//      rls|<AnalysisArgsCrc>;<ExtPathArgsCrc>;<ExtLinkPathArgsCrc>;<ProjPathArgsCrc>;<LinkArgsCrc>;<OtherArgsCrc>
//      <crc>|<args>
//
// Example:
//      ana|6802;11603
//      dbg|6802;7565
//      6802|C:\foo\include;;-std=c++11
//      11603|C:\foo\include;;-O0
//
// <analysis args crc> This is a CRC of the arguments that affect analysis.
//      This is a crc of the ext paths and the proj paths combined.
// <ext path crc>       This includes args -I, -ER, -D, etc. This is used as an index to analysis files.
// <ext link path crc>  This includes -L. This indicates when to rescan external directories for link paths.
// <link args crc>      This is a CRC of link arguments. This indicates when to redo a link to rebuild executables.
// <other args crc>     This is a CRC of all remaining arguments. This indicates when to redo the whole build.
// <crc>|args           This is a CRC as a string, and the args are the arguments as a string.

class BuildConfig
    {
    public:
        // WARNING: These must match the order in the build config file.
        enum CrcTypes
            {
            CT_AnalysisArgsCrc,         // CRC of ext paths and proj paths

            CT_ExtPathArgsCrc,          // CRC of args that affect the ext paths
                CT_FirstCrc=CT_ExtPathArgsCrc,
            CT_ProjPathArgsCrc,         // CRC of args that affect the proj paths
            CT_LinkArgsCrc,
            CT_OtherArgsCrc,            // All other args such as optimization args
                CT_LastCrc=CT_OtherArgsCrc
            };

        // Get the analysis path of the specified build type.
        std::string getAnalysisPath(OovStringRef const buildType=BuildConfigAnalysis) const;
        std::string getAnalysisPath(OovStringRef const buildType,
                OovStringRef const crcStr) const;
        std::string getIncDepsFilePath(OovStringRef const buildType) const;
        std::string getComponentsFilePath(OovStringRef const buildType) const;
        std::string getCrcAsStr(OovStringRef const buildType, CrcTypes crcType) const;
        std::string getBuildConfigFilename() const;

    protected:
        NameValueFile mConfigFile;
        BuildConfig();
    };

class BuildConfigReader:public BuildConfig
    {
    };

#endif /* BUILDCONFIGREADER_H_ */

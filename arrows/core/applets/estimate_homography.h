/*ckwg +29
 * Copyright 2019 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef KWIVER_ARROWS_CORE_APPLETS_ESTIMATE_HOMOGRAPHY_H
#define KWIVER_ARROWS_CORE_APPLETS_ESTIMATE_HOMOGRAPHY_H

#include <tools/kwiver_applet.h>

#include <arrows/core/applets/kwiver_algo_core_applets_export.h>

#include <string>
#include <vector>

namespace kwiver {
namespace arrows {
namespace core {

class KWIVER_ALGO_CORE_APPLETS_EXPORT estimate_homography
  : public kwiver::tools::kwiver_applet
{
public:
  estimate_homography(){}
  virtual ~estimate_homography() = default;

  static constexpr char const* name = "estimate-homography";
  static constexpr char const* description =
    "Estmate a homography matrix that registers two images\n\n";


  virtual int run( const std::vector<std::string>& argv );
  virtual void usage( std::ostream& outstream ) const;

protected:

private:

}; // end of class

} } } // end namespace

#endif /* KWIVER_ARROWS_CORE_APPLETS_ESTIMATE_HOMOGRAPHY_H */

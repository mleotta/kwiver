/*ckwg +29
 * Copyright 2016, 2020 by Kitware, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
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

/**
 * \file
 * \brief Matlab algorithm registration implementation
 */

#include <arrows/matlab/kwiver_algo_matlab_plugin_export.h>
#include <vital/algo/algorithm_factory.h>

#include <arrows/matlab/matlab_image_object_detector.h>
#include <arrows/matlab/matlab_image_filter.h>
#include <arrows/matlab/matlab_detection_output.h>

namespace kwiver {
namespace arrows {
namespace matlab {

extern "C"
KWIVER_ALGO_MATLAB_PLUGIN_EXPORT
void
register_factories( ::kwiver::vital::plugin_loader& vpm )
{
  ::kwiver::vital::algorithm_registrar reg( vpm, "arrows.matlab" );

  if (reg.is_module_loaded())
  {
    return;
  }

  reg.register_algorithm< ::kwiver::arrows::matlab::matlab_image_object_detector >();
  reg.register_algorithm< ::kwiver::arrows::matlab::matlab_image_filter >();
  reg.register_algorithm< ::kwiver::arrows::matlab::matlab_detection_output >();

  reg.mark_module_as_loaded();
}

} } } // end namespace

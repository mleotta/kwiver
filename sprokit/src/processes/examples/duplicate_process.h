/*ckwg +29
 * Copyright 2013-2017, 2020 by Kitware, Inc. All Rights Reserved. Please refer to
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

#ifndef SPROKIT_PROCESSES_EXAMPLES_DUPLICATE_PROCESS_H
#define SPROKIT_PROCESSES_EXAMPLES_DUPLICATE_PROCESS_H

#include "processes_examples_export.h"

#include <sprokit/pipeline/process.h>

/**
 * \file duplicate_process.h
 *
 * \brief Declaration of the duplicate process.
 */

namespace sprokit
{

/**
 * \class duplicate_process
 *
 * \brief Duplicates input to a single output.
 *
 * \process Duplicates input to a single output.
 *
 * \iports
 *
 * \iport{input} Arbitrary input data.
 *
 * \oports
 *
 * \oport{duplicate} Duplicated input data.
 *
 * \configs
 *
 * \config{copies} The number of copies to make per input.
 *
 * \ingroup examples
 */
class PROCESSES_EXAMPLES_NO_EXPORT duplicate_process
  : public process
{
public:
  PLUGIN_INFO( "duplicate",
               "A process which duplicates input" );
  /**
   * \brief Constructor.
   *
   * \param config The configuration for the process.
   */
  duplicate_process(kwiver::vital::config_block_sptr const& config);

  /**
   * \brief Destructor.
   */
  ~duplicate_process();

protected:
  /**
   * \brief Configure the process.
   */
  void _configure() override;

  /**
   * \brief Step the process.
   */
  void _step() override;

private:
  class priv;
  std::unique_ptr<priv> d;
};

}

#endif // SPROKIT_PROCESSES_EXAMPLES_DUPLICATE_PROCESS_H

/*ckwg +29
 * Copyright 2017 by Kitware, Inc.
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
 *
 * \brief Common tests for all track set implementations
 *
 * These test functions are in a header rather than a source file so that
 * anyone who writes a new track_set_implementation can run the same tests.
 */

#ifndef KWIVER_VITAL_TEST_TEST_TRACK_SET_H_
#define KWIVER_VITAL_TEST_TEST_TRACK_SET_H_


#include <vital/types/track_set.h>

namespace kwiver {
namespace vital {
namespace testing {

// Make a very small example track set
track_set_sptr
make_simple_track_set()
{
  unsigned track_id = 0;

  std::vector< track_sptr > test_tracks;

  auto test_state1 = std::make_shared<track_state>( 1 );
  auto test_state2 = std::make_shared<track_state>( 4 );
  auto test_state3 = std::make_shared<track_state>( 9 );

  test_tracks.push_back( track::make() ) ;
  test_tracks.back()->append( test_state1 );
  test_tracks.back()->set_id( track_id++ );

  test_tracks.push_back( track::make() ) ;
  test_tracks.back()->append( test_state1->clone() );
  test_tracks.back()->set_id( track_id++ );

  // skip some track ids
  track_id = 5;

  test_tracks.push_back( track::make() ) ;
  test_tracks.back()->append( test_state2 );
  test_tracks.back()->set_id( track_id++ );

  test_tracks.push_back( track::make() ) ;
  test_tracks.back()->append( test_state3 );
  test_tracks.back()->set_id( track_id++ );

  test_tracks[0]->append( test_state2->clone() );
  test_tracks[0]->append( test_state3->clone() );
  test_tracks[1]->append( test_state2->clone() );
  test_tracks[2]->append( test_state3->clone() );

  return std::make_shared<track_set>(test_tracks);
}


// Run the unit tests for track_set accessor functions
//
// This test assumes the tracks in the set correspond to those
// generated in the above make_simple_track_set() function.
void
test_track_set_accessors(track_set_sptr test_set)
{
  TEST_EQUAL("Set empty", test_set->empty(), false);
  TEST_EQUAL("Total set size", test_set->size(), 4);

  TEST_EQUAL("Active set size (-1)", test_set->active_tracks(-1).size(), 3);
  TEST_EQUAL("Active set size (4)", test_set->active_tracks(4).size(), 3);
  TEST_EQUAL("Active set size (1)", test_set->active_tracks(1).size(), 2);
  TEST_EQUAL("Inactive set size (4)", test_set->inactive_tracks(4).size(), 1);
  TEST_EQUAL("Inactive set size (1)", test_set->inactive_tracks(1).size(), 2);

  TEST_EQUAL("Get Track ID (2)", test_set->get_track(2) == nullptr, true);
  TEST_EQUAL("Get Track ID (4)", test_set->get_track(5)->id(), 5);

  std::set<frame_id_t> all_frames = test_set->all_frame_ids();
  std::set<frame_id_t> true_frames({1,4,9});
  TEST_EQUAL("Access all frame IDs", std::equal(all_frames.begin(), all_frames.end(),
                                                true_frames.begin()), true);

  std::set<track_id_t> all_track_ids = test_set->all_track_ids();
  std::set<track_id_t> true_track_ids({0,1,5,6});
  TEST_EQUAL("Access all track IDs", std::equal(all_track_ids.begin(), all_track_ids.end(),
                                                true_track_ids.begin()), true);

  TEST_EQUAL("First frame ID", test_set->first_frame(), 1);
  TEST_EQUAL("Last frame ID", test_set->last_frame(), 9);
  TEST_EQUAL("Terminated set size", test_set->terminated_tracks(-1).size(), 3);
  TEST_EQUAL("New set size (4)", test_set->new_tracks(4).size(), 1);
  TEST_EQUAL("New set size (-2)", test_set->new_tracks(-2).size(), 0);

  TEST_EQUAL("Percentage tracked", test_set->percentage_tracked(-1,-6), 0.5);
}


} // end namespace testing
} // end namespace vital
} // end namespace kwiver

#endif // KWIVER_VITAL_TEST_TEST_TRACK_SET_H_

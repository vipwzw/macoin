//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "uint256.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(Checkpoints_tests)

BOOST_AUTO_TEST_CASE(sanity)
{
    uint256 p11111 = uint256("0x00000004e1016f6e28a80db812c9d862270aba3a5cf50abbbf5fc939dfeb2a41");
    //uint256 p134444 = uint256("0x00000000000005b12ffd4cd315cd34ffd4a594f430ac814c91184a0d42d2b0fe");
    BOOST_CHECK(Checkpoints::CheckBlock(11111, p11111));
    //BOOST_CHECK(Checkpoints::CheckBlock(134444, p134444));

    
    // Wrong hashes at checkpoints should fail:
    //BOOST_CHECK(!Checkpoints::CheckBlock(11111, p134444));
    //BOOST_CHECK(!Checkpoints::CheckBlock(134444, p11111));

    // ... but any hash not at a checkpoint should succeed:
    //BOOST_CHECK(Checkpoints::CheckBlock(11111+1, p134444));
    //BOOST_CHECK(Checkpoints::CheckBlock(134444+1, p11111));

    //BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 134444);
}    

BOOST_AUTO_TEST_SUITE_END()

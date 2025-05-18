#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // 直接截断即可
  return Wrap32 { (uint32_t)( n + zero_point.raw_value_ ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t this_low32 = this->raw_value_ - zero_point.raw_value_;
  uint64_t checkpoint_low32 = checkpoint & MASK_LOW_32;
  uint64_t res = ( checkpoint & MASK_HIGH_32 ) | this_low32;
  if ( this_low32 > checkpoint_low32 and ( this_low32 - checkpoint_low32 ) > ( BASE / 2 ) and res >= BASE ) {
    return res - BASE;
  }
  if ( this_low32 < checkpoint_low32 and ( checkpoint_low32 - this_low32 ) > ( BASE / 2 ) and res < MASK_HIGH_32 ) {
    return res + BASE;
  }
  return res;
}

// F-N (ssif_underflow_neg): unsigned underflow on
//   `(_tx_offset - StartPartSize) / RemPartSize`
// in src/nv/ssif/ssif.cpp:408 — Ssif::smbus_block_read, ReadMulti branch.
//
// Production code:
//
//   case ReadMulti: {
//       auto& part = MiddlePartData::from(tx.data);
//       if ((_tx_size > MaxPartSize) && (_tx_size <= MaxPayloadSize)
//           && (_tx_offset > 0) && (_tx_offset < _tx_size)) {
//           const size_t rem_size = _tx_size - _tx_offset;
//           if (rem_size <= RemPartSize) {
//               tx.size        = rem_size + 1;
//               part.block_num = LastReadBlock;
//           }
//           else {
//               tx.size        = MaxPartSize;
//               // Guard is `_tx_offset > 0`, NOT `_tx_offset >= StartPartSize`.
//               // For _tx_offset ∈ [1, StartPartSize-1] = [1, 29], the
//               // subtraction wraps in size_t.
//               part.block_num = (_tx_offset - StartPartSize) / RemPartSize;
//           }
//           ...
//       }
//   }
//
// Reachability: the only production paths setting `_tx_offset` are
//   (a) ReadStart  → _tx_offset = StartPartSize = 30   (fine)
//                  _tx_offset = _tx_size              (only when _tx_size <= MaxPartSize,
//                                                      so the outer ReadMulti guard
//                                                      `_tx_size > MaxPartSize` rejects)
//   (b) ReadMulti  → _tx_offset += tx.size - 1        (monotonically increasing from 30)
//   (c) ReadRetry  → _tx_offset = (MaxPartSize-StartPartSize) +
//                                  block_num * (MaxPartSize-RemPartSize)
//                                = 2 + block_num
//                    where block_num is rx.data[0] (attacker-supplied).
//                    Range: [2, 257]; intersects [1, 29] = the underflow window
//                    when block_num ∈ [0, 27].
//
// So a hostile BMC that sends a valid SMBus Block Write with cmd=ReadRetry
// and data[0] ∈ [0, 27] sets _tx_offset to a small value; the next
// ReadMulti read phase enters the else-branch when the message is large
// enough that rem_size > RemPartSize (= 31), i.e. _tx_size - _tx_offset > 31.
// The subtraction `_tx_offset - StartPartSize` then wraps modulo 2^64.
//
// Severity: NOT a memory-safety defect — `block_num` is uint8_t and the
// wrapped quotient is truncated, so no OOB access. But the SMBus response
// carries a corrupted `block_num` field; the BMC sees malformed protocol
// and the in-flight multi-block read is desynchronised.
//
// Suggested fix: replace the guard `(_tx_offset > 0)` with
// `(_tx_offset >= StartPartSize)` so only ReadStart / ReadMulti
// successor states can enter the else-branch.
//
// Expected: VERIFICATION FAILED ("arithmetic overflow on sub")
//           CEX: _tx_offset ∈ [1, 29], _tx_size > MaxPartSize.

#include <cstddef>
#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
size_t   nondet_size();
}

// Mirror the ssif.h compile-time constants verbatim.
constexpr static size_t MaxPartSize    = 32;
constexpr static size_t StartPartSize  = MaxPartSize - 2;   // 30
constexpr static size_t RemPartSize    = MaxPartSize - 1;   // 31
constexpr static size_t MaxPayloadSize = 254;

int main()
{
    // _tx_offset reachable values: 0, [2,257] via ReadRetry, or
    // {StartPartSize, StartPartSize + k*RemPartSize, _tx_size} via
    // ReadStart/ReadMulti. Of these, [1, 29] is reachable only via
    // ReadRetry with block_num ∈ [0, 27] — that's the underflow window.
    // We model the worst case (ReadRetry-set state) by allowing any
    // _tx_offset value the production code can produce.
    size_t _tx_offset = nondet_size();
    size_t _tx_size   = nondet_size();

    // Outer ReadMulti else-branch entry conditions (ssif.cpp:399-407).
    __ESBMC_assume(_tx_size  > MaxPartSize);
    __ESBMC_assume(_tx_size  <= MaxPayloadSize);
    __ESBMC_assume(_tx_offset > 0);
    __ESBMC_assume(_tx_offset < _tx_size);
    const size_t rem_size = _tx_size - _tx_offset;
    __ESBMC_assume(rem_size > RemPartSize);   // forces the else-branch.

    // The production statement under test. With --unsigned-overflow-check
    // ESBMC reports the size_t subtraction overflow when _tx_offset < 30.
    const size_t block_num = (_tx_offset - StartPartSize) / RemPartSize;
    (void)block_num;

    return 0;
}

// Expected: VERIFICATION FAILED — "arithmetic overflow on sub"
//   CEX example: _tx_offset = 2, _tx_size = 34.

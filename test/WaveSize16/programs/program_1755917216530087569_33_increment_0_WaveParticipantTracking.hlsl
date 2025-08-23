RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 8);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 2);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  }
}

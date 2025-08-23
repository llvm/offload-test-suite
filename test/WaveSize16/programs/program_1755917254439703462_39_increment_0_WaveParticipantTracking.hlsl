RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  if ((WaveGetLaneIndex() >= 10)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 6);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveMin(2));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 2);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 0))) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 2);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  }
  }
}

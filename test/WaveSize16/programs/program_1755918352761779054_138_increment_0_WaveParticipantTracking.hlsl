RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(1));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 2);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if ((WaveGetLaneIndex() == 13)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 0);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
    result = (result + WaveActiveMax(3));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 4);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 0))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 2);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  }
  }
  }
}

RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13))) {
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveSum(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        break;
      }
    case 1: {
        if (((WaveGetLaneIndex() % 2) == 0)) {
          result = (result + WaveActiveSum(2));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        break;
      }
    }
    if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMin(8));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 0);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
  } else {
  if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(result));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 1);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  }
}

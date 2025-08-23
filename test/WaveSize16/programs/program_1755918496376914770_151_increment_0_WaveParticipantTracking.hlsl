RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 8);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 0);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(1));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
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
    case 2: {
        if (true) {
          result = (result + WaveActiveSum(3));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 2);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        break;
      }
    default: {
        result = (result + WaveActiveSum(99));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        break;
      }
    }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      if ((WaveGetLaneIndex() == 4)) {
        result = (result + WaveActiveMin(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      if ((WaveGetLaneIndex() == 10)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      if ((counter0 == 2)) {
        break;
      }
    }
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveSum(result));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 0);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
  }
}

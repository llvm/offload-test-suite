RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 4);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      break;
    }
  case 1: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 4);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 4))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
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
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 4);
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
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveSum(1));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 8);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
    result = (result + WaveActiveMin(2));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 2);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 0);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveSum(4));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 0);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  }
  }
  }
}

RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(2));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if ((WaveGetLaneIndex() == 5)) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((WaveGetLaneIndex() < 6)) {
            if ((WaveGetLaneIndex() < 3)) {
              result = (result + WaveActiveSum(1));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
          } else {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveSum(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if ((i0 == 1)) {
          continue;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
    }
    break;
  }
  case 1: {
    if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 3))) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMin(10));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 1))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
    }
    break;
  }
  case 2: {
    if (true) {
      result = (result + WaveActiveSum(3));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 4);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    break;
  }
  case 3: {
    if ((WaveGetLaneIndex() < 20)) {
      result = (result + WaveActiveSum(4));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 4);
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
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMin(9));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 3);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
    }
  case 1: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 6);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 6);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 6);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
    }
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 16);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      break;
    }
  }
}

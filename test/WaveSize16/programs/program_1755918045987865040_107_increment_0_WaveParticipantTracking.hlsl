RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
    if ((WaveGetLaneIndex() == 8)) {
      result = (result + WaveActiveSum(5));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 1);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if ((WaveGetLaneIndex() >= 11)) {
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveMin(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveMax(2));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 3);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
    }
    if ((WaveGetLaneIndex() == 10)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 1);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    if ((i0 == 1)) {
      continue;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 2);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
          if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
      }
      break;
    }
  case 1: {
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
      }
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 2);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          break;
        }
      case 1: {
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 6))) {
            if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMin(result));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 1);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
            if (((WaveGetLaneIndex() & 1) == 0)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMin(4));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 2);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
              if (((WaveGetLaneIndex() & 1) == 1)) {
                result = (result + WaveActiveMin(WaveGetLaneIndex()));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 0);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
            }
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 4))) {
              result = (result + WaveActiveMax(5));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 1);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
          }
          break;
        }
      case 2: {
          uint counter2 = 0;
          while ((counter2 < 3)) {
            counter2 = (counter2 + 1);
            if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 9))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
              if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
                result = (result + WaveActiveSum(result));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 1);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
            }
            if ((counter2 == 2)) {
              break;
            }
          }
          break;
        }
      }
      break;
    }
  case 1: {
      uint counter3 = 0;
      while ((counter3 < 3)) {
        counter3 = (counter3 + 1);
        if ((WaveGetLaneIndex() == 15)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((WaveGetLaneIndex() >= 9)) {
            if ((WaveGetLaneIndex() < 2)) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
          }
          if ((i4 == 1)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() == 4)) {
          result = (result + WaveActiveMax(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      break;
    }
  }
}

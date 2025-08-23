RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
        if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        uint counter0 = 0;
        while ((counter0 < 3)) {
          counter0 = (counter0 + 1);
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((counter0 == 2)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
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
  case 2: {
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveSum(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 1);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
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
  }
  switch ((WaveGetLaneIndex() % 4)) {
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
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(result));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMax(result));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      } else {
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
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
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11))) {
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
          }
          break;
        }
      }
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
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
    if ((WaveGetLaneIndex() >= 11)) {
      if ((WaveGetLaneIndex() >= 15)) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        if ((WaveGetLaneIndex() == 0)) {
          result = (result + WaveActiveSum(6));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if ((WaveGetLaneIndex() >= 14)) {
          if ((WaveGetLaneIndex() >= 14)) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 1);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((WaveGetLaneIndex() < 2)) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        } else {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(8));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((WaveGetLaneIndex() == 5)) {
        result = (result + WaveActiveMin(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
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
  if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 1))) {
    if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveSum(7));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 1);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    uint counter5 = 0;
    while ((counter5 < 2)) {
      counter5 = (counter5 + 1);
      if ((WaveGetLaneIndex() == 7)) {
        result = (result + WaveActiveMin(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      uint counter6 = 0;
      while ((counter6 < 3)) {
        counter6 = (counter6 + 1);
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        switch ((WaveGetLaneIndex() % 2)) {
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
        default: {
            result = (result + WaveActiveSum(99));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            break;
          }
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
    }
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
      result = (result + WaveActiveMin(result));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 0);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
  } else {
  if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 4))) {
    result = (result + WaveActiveSum(result));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 2);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  uint counter7 = 0;
  while ((counter7 < 2)) {
    counter7 = (counter7 + 1);
    if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMax(result));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 2);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14))) {
      if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      for (uint i8 = 0; (i8 < 3); i8 = (i8 + 1)) {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
    }
    if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveSum(result));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 4);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
  }
  }
}

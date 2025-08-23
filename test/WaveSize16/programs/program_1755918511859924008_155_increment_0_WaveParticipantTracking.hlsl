RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          switch ((WaveGetLaneIndex() % 3)) {
          case 0: {
              uint counter0 = 0;
              while ((counter0 < 3)) {
                counter0 = (counter0 + 1);
                if ((WaveGetLaneIndex() >= 8)) {
                  result = (result + WaveActiveSum(WaveGetLaneIndex()));
                  uint _participantCount = WaveActiveSum(1);
                  bool _isCorrect = (_participantCount == 1);
                  _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
                }
                if ((WaveGetLaneIndex() < 2)) {
                  result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
                  uint _participantCount = WaveActiveSum(1);
                  bool _isCorrect = (_participantCount == 1);
                  _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
                }
              }
              break;
            }
          case 1: {
              if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
                if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
                  result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
                  uint _participantCount = WaveActiveSum(1);
                  bool _isCorrect = (_participantCount == 0);
                  _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
                }
                if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
                  result = (result + WaveActiveMax(WaveGetLaneIndex()));
                  uint _participantCount = WaveActiveSum(1);
                  bool _isCorrect = (_participantCount == 0);
                  _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
                }
              }
              break;
            }
          case 2: {
              if (((WaveGetLaneIndex() & 1) == 1)) {
                if (((WaveGetLaneIndex() & 1) == 1)) {
                  result = (result + WaveActiveMax(4));
                  uint _participantCount = WaveActiveSum(1);
                  bool _isCorrect = (_participantCount == 0);
                  _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
                }
              }
              break;
            }
          }
          break;
        }
      case 1: {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 4))) {
            if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMin(result));
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
                  bool _isCorrect = (_participantCount == 1);
                  _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
                }
                break;
              }
            case 2: {
                if (true) {
                  result = (result + WaveActiveSum(3));
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
            if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
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
            bool _isCorrect = (_participantCount == 1);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          break;
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
      if (true) {
        result = (result + WaveActiveSum(3));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 4);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      break;
    }
  case 3: {
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMin(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if ((WaveGetLaneIndex() < 1)) {
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          switch ((WaveGetLaneIndex() % 4)) {
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
              if (true) {
                result = (result + WaveActiveSum(3));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 0);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
            }
          case 3: {
              if ((WaveGetLaneIndex() < 20)) {
                result = (result + WaveActiveSum(4));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 0);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
              break;
            }
          }
        } else {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 2);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if ((WaveGetLaneIndex() < 5)) {
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((WaveGetLaneIndex() < 5)) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 1);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
      }
      if ((WaveGetLaneIndex() >= 13)) {
        result = (result + WaveActiveSum(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 8)) {
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMin(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMax(5));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
            if ((i3 == 1)) {
              break;
            }
          }
        }
        if ((WaveGetLaneIndex() >= 8)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 2);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      } else {
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if ((WaveGetLaneIndex() >= 9)) {
            if ((WaveGetLaneIndex() >= 11)) {
              result = (result + WaveActiveSum(result));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
            if ((WaveGetLaneIndex() < 5)) {
              result = (result + WaveActiveMax(result));
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
          if (true) {
            result = (result + WaveActiveSum(3));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          break;
        }
      case 3: {
          if ((WaveGetLaneIndex() == 14)) {
            if ((WaveGetLaneIndex() == 10)) {
              result = (result + WaveActiveMax(3));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
            if ((WaveGetLaneIndex() == 4)) {
              result = (result + WaveActiveSum(1));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
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
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMax(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
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
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
          if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
    } else {
    if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 0);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
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
  if ((WaveGetLaneIndex() == 1)) {
    if ((WaveGetLaneIndex() == 2)) {
      result = (result + WaveActiveMin(result));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 0);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    uint counter4 = 0;
    while ((counter4 < 3)) {
      counter4 = (counter4 + 1);
      for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(9));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((counter4 == 2)) {
        break;
      }
    }
  }
}

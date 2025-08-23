[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMin(result));
        }
        uint counter0 = 0;
        while ((counter0 < 3)) {
          counter0 = (counter0 + 1);
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(result));
          }
        }
      } else {
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum(5));
      }
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(result));
        }
        if ((i1 == 1)) {
          continue;
        }
      }
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
      }
    }
    break;
  }
  case 1: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        break;
      }
    case 1: {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        break;
      }
    case 2: {
        if (true) {
          result = (result + WaveActiveSum(3));
        }
        break;
      }
    }
    break;
  }
  case 2: {
    if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 10))) {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveMax(10));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
      }
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveSum(result));
      }
    } else {
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
      }
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveSum(result));
      }
      if ((counter2 == 2)) {
        break;
      }
    }
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveSum(result));
    }
  }
  break;
  }
  case 3: {
    if ((WaveGetLaneIndex() < 20)) {
      result = (result + WaveActiveSum(4));
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}

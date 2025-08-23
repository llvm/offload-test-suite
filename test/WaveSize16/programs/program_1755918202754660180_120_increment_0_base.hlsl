[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 3)) {
    counter0 = (counter0 + 1);
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
      result = (result + WaveActiveMax(result));
    }
    for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
      }
      if ((WaveGetLaneIndex() >= 11)) {
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveMin(result));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((i1 == 2)) {
        break;
      }
    }
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
    }
    if ((counter0 == 2)) {
      break;
    }
  }
  if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      if ((WaveGetLaneIndex() < 2)) {
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
        }
        if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 9))) {
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveSum(result));
          }
        }
      } else {
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveSum(result));
      }
      uint counter4 = 0;
      while ((counter4 < 3)) {
        counter4 = (counter4 + 1);
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
        }
      }
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((WaveGetLaneIndex() == 5)) {
      result = (result + WaveActiveMin(result));
    }
    if ((counter3 == 2)) {
      break;
    }
  }
  }
}

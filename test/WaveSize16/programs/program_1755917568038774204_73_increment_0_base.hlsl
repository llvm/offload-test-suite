[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
      result = (result + WaveActiveSum(result));
    }
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 7))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
      }
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 3))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
      }
    }
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  }
}

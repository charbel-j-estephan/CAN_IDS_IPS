


    // !!! This file is generated using emlearn !!!

    #include <stdint.h>

// Where the model's code goes. By default these are ordinary functions. A
// firmware can define CAN_IDS_FN / CAN_IDS_PUBLIC before including this file
// to put them in a linker section, for example RAM (stm32_ids/src/classify.c).
#ifndef CAN_IDS_FN
#define CAN_IDS_FN static inline
#endif
#ifndef CAN_IDS_PUBLIC
#define CAN_IDS_PUBLIC
#endif

    

CAN_IDS_FN int32_t can_ids_tree_0(const float *features, int32_t features_length) {
          if (features[5] < 143.500000f) {
              if (features[0] < 0.500000f) {
                  if (features[7] < 2.500000f) {
                      return 1;
                  } else {
                      return 2;
                  }
              } else {
                  if (features[7] < 148.500000f) {
                      if (features[3] < 191.500000f) {
                          if (features[4] < 154.500000f) {
                              if (features[4] < 0.500000f) {
                                  if (features[6] < 78.500000f) {
                                      if (features[3] < 149.500000f) {
                                          if (features[7] < 134.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 133.500000f) {
                                          if (features[2] < 237.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 32.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 34.500000f) {
                                      if (features[0] < 1680.500000f) {
                                          if (features[6] < 160.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 2008.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[7] < 140.500000f) {
                                          if (features[0] < 1088.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 53.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 105.500000f) {
                                  if (features[7] < 36.500000f) {
                                      if (features[6] < 122.500000f) {
                                          if (features[4] < 252.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 18.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 62.500000f) {
                                          if (features[2] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 42.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[0] < 1221.500000f) {
                                          if (features[2] < 231.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[3] < 251.500000f) {
                              if (features[5] < 7.500000f) {
                                  if (features[6] < 11.500000f) {
                                      if (features[0] < 1215.500000f) {
                                          if (features[4] < 144.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 6.500000f) {
                                          if (features[5] < 1.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[6] < 1.500000f) {
                                          return 0;
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 19.500000f) {
                                          if (features[0] < 810.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 497.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 7.500000f) {
                                  if (features[7] < 2.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[6] < 25.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[0] < 1261.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[5] < 0.500000f) {
                          if (features[4] < 47.500000f) {
                              if (features[6] < 254.500000f) {
                                  if (features[3] < 45.500000f) {
                                      if (features[4] < 17.500000f) {
                                          if (features[2] < 228.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 0;
                              }
                          } else {
                              if (features[6] < 16.500000f) {
                                  if (features[0] < 525.500000f) {
                                      if (features[2] < 7.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[6] < 254.500000f) {
                              if (features[7] < 150.500000f) {
                                  if (features[6] < 9.500000f) {
                                      if (features[0] < 624.500000f) {
                                          if (features[2] < 51.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 15.500000f) {
                                      if (features[2] < 0.500000f) {
                                          if (features[5] < 63.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 48.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 0.500000f) {
                                          if (features[3] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 17.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 48.500000f) {
                                  if (features[4] < 178.500000f) {
                                      if (features[5] < 17.500000f) {
                                          return 2;
                                      } else {
                                          if (features[3] < 165.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[5] < 127.500000f) {
                                      if (features[7] < 254.500000f) {
                                          return 2;
                                      } else {
                                          if (features[2] < 33.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 151.500000f) {
                                          if (features[5] < 129.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 13.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[7] < 36.500000f) {
                  if (features[0] < 790.500000f) {
                      if (features[3] < 41.500000f) {
                          if (features[4] < 36.500000f) {
                              if (features[0] < 788.500000f) {
                                  if (features[2] < 2.500000f) {
                                      if (features[5] < 155.500000f) {
                                          if (features[0] < 291.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 4;
                              }
                          } else {
                              if (features[6] < 2.500000f) {
                                  if (features[2] < 3.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[2] < 7.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[2] < 0.500000f) {
                              if (features[4] < 64.500000f) {
                                  if (features[0] < 423.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      }
                  } else {
                      if (features[2] < 1.500000f) {
                          if (features[3] < 68.500000f) {
                              if (features[4] < 123.500000f) {
                                  if (features[6] < 150.500000f) {
                                      if (features[0] < 1587.500000f) {
                                          if (features[5] < 172.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[7] < 2.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[5] < 254.500000f) {
                                  if (features[6] < 1.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 70.500000f) {
                                      if (features[7] < 0.500000f) {
                                          if (features[6] < 106.500000f) {
                                              return 0;
                                          } else {
                                              return 3;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 0;
                                  }
                              }
                          }
                      } else {
                          if (features[2] < 16.500000f) {
                              if (features[0] < 1201.500000f) {
                                  if (features[6] < 125.500000f) {
                                      if (features[0] < 1086.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 1090.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[0] < 1201.500000f) {
                                  if (features[0] < 1200.500000f) {
                                      if (features[5] < 254.500000f) {
                                          return 2;
                                      } else {
                                          if (features[4] < 105.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 250.500000f) {
                                          if (features[7] < 3.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 117.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 0.500000f) {
                                      if (features[4] < 4.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              } else {
                  if (features[3] < 130.500000f) {
                      if (features[5] < 254.500000f) {
                          if (features[7] < 160.500000f) {
                              if (features[5] < 144.500000f) {
                                  if (features[7] < 102.500000f) {
                                      if (features[6] < 6.500000f) {
                                          if (features[0] < 556.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 3.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[2] < 0.500000f) {
                                      if (features[4] < 164.500000f) {
                                          if (features[3] < 125.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 497.500000f) {
                                  if (features[2] < 0.500000f) {
                                      if (features[0] < 494.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[0] < 2024.500000f) {
                                      return 2;
                                  } else {
                                      if (features[4] < 6.500000f) {
                                          if (features[5] < 172.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[6] < 126.500000f) {
                              if (features[4] < 96.500000f) {
                                  if (features[2] < 32.500000f) {
                                      if (features[7] < 38.500000f) {
                                          if (features[3] < 42.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 31.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 0.500000f) {
                                          if (features[0] < 589.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 95.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[3] < 105.500000f) {
                                  if (features[3] < 3.500000f) {
                                      if (features[2] < 68.500000f) {
                                          if (features[4] < 82.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 237.500000f) {
                                          if (features[3] < 104.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 220.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[4] < 11.500000f) {
                                      if (features[0] < 360.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[5] < 254.500000f) {
                          if (features[5] < 144.500000f) {
                              if (features[0] < 501.500000f) {
                                  if (features[2] < 0.500000f) {
                                      if (features[6] < 80.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[4] < 40.500000f) {
                                  if (features[2] < 0.500000f) {
                                      if (features[0] < 552.500000f) {
                                          if (features[4] < 39.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[0] < 497.500000f) {
                                      if (features[0] < 496.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 63.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[6] < 119.500000f) {
                              if (features[3] < 145.500000f) {
                                  if (features[0] < 311.500000f) {
                                      if (features[0] < 242.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[3] < 144.500000f) {
                                  if (features[0] < 341.500000f) {
                                      if (features[7] < 148.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[7] < 130.500000f) {
                                      if (features[7] < 129.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 180.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_1(const float *features, int32_t features_length) {
          if (features[6] < 40.500000f) {
              if (features[0] < 0.500000f) {
                  if (features[7] < 2.500000f) {
                      return 1;
                  } else {
                      return 2;
                  }
              } else {
                  if (features[5] < 48.500000f) {
                      if (features[4] < 128.500000f) {
                          if (features[5] < 20.500000f) {
                              if (features[7] < 148.500000f) {
                                  if (features[3] < 192.500000f) {
                                      if (features[0] < 1520.500000f) {
                                          if (features[5] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 1211.500000f) {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[5] < 0.500000f) {
                                      if (features[3] < 52.500000f) {
                                          if (features[4] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 0.500000f) {
                                          if (features[0] < 272.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 8.500000f) {
                                  if (features[7] < 154.500000f) {
                                      if (features[4] < 116.500000f) {
                                          if (features[3] < 55.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 123.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[2] < 5.500000f) {
                                      if (features[0] < 853.500000f) {
                                          if (features[0] < 480.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[3] < 134.500000f) {
                                          if (features[3] < 126.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[3] < 136.500000f) {
                              if (features[3] < 40.500000f) {
                                  if (features[2] < 15.500000f) {
                                      if (features[7] < 46.500000f) {
                                          if (features[0] < 775.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 1200.500000f) {
                                          if (features[6] < 36.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 0.500000f) {
                                      if (features[7] < 39.500000f) {
                                          if (features[0] < 1526.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 10.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[7] < 53.500000f) {
                                          if (features[3] < 122.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 126.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 0.500000f) {
                                  if (features[3] < 151.500000f) {
                                      if (features[7] < 16.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[7] < 1.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[6] < 15.500000f) {
                                      if (features[2] < 0.500000f) {
                                          if (features[3] < 243.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 0.500000f) {
                                          if (features[4] < 141.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[0] < 1349.500000f) {
                          if (features[4] < 16.500000f) {
                              if (features[3] < 142.500000f) {
                                  if (features[3] < 131.500000f) {
                                      if (features[6] < 0.500000f) {
                                          if (features[2] < 254.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 8.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 300.500000f) {
                                          return 2;
                                      } else {
                                          if (features[7] < 144.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[2] < 215.500000f) {
                                      if (features[2] < 0.500000f) {
                                          if (features[7] < 150.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 0.500000f) {
                                          if (features[0] < 1341.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 188.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 241.500000f) {
                                  if (features[2] < 0.500000f) {
                                      if (features[6] < 24.500000f) {
                                          if (features[6] < 20.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 49.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 1200.500000f) {
                                          return 2;
                                      } else {
                                          if (features[7] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 0.500000f) {
                                      if (features[5] < 254.500000f) {
                                          if (features[7] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 33.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 0.500000f) {
                                          if (features[7] < 139.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          return 2;
                      }
                  }
              }
          } else {
              if (features[5] < 105.500000f) {
                  if (features[7] < 2.500000f) {
                      if (features[0] < 1681.500000f) {
                          if (features[7] < 0.500000f) {
                              if (features[3] < 41.500000f) {
                                  if (features[0] < 665.500000f) {
                                      return 2;
                                  } else {
                                      if (features[4] < 38.500000f) {
                                          return 0;
                                      } else {
                                          if (features[5] < 4.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[4] < 97.500000f) {
                                      if (features[4] < 55.500000f) {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 10.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[1] < 6.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 149.500000f) {
                                  if (features[5] < 0.500000f) {
                                      if (features[2] < 207.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 5.500000f) {
                                      if (features[7] < 1.500000f) {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 0.500000f) {
                                          if (features[3] < 44.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          return 2;
                      }
                  } else {
                      if (features[3] < 128.500000f) {
                          if (features[5] < 48.500000f) {
                              if (features[6] < 254.500000f) {
                                  if (features[7] < 134.500000f) {
                                      if (features[3] < 126.500000f) {
                                          if (features[4] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 1.500000f) {
                                          if (features[4] < 16.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 126.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[3] < 90.500000f) {
                                      if (features[2] < 4.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 1096.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 642.500000f) {
                                          if (features[3] < 116.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 249.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[5] < 48.500000f) {
                              if (features[7] < 21.500000f) {
                                  if (features[6] < 156.500000f) {
                                      if (features[2] < 53.500000f) {
                                          if (features[3] < 157.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 54.500000f) {
                                          if (features[7] < 19.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[0] < 608.500000f) {
                                      if (features[6] < 254.500000f) {
                                          if (features[0] < 305.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 47.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 195.500000f) {
                                          if (features[3] < 131.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 53.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              return 2;
                          }
                      }
                  }
              } else {
                  if (features[2] < 35.500000f) {
                      if (features[6] < 107.500000f) {
                          if (features[2] < 1.500000f) {
                              if (features[7] < 0.500000f) {
                                  if (features[0] < 1229.500000f) {
                                      if (features[6] < 106.500000f) {
                                          return 0;
                                      } else {
                                          return 3;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 89.500000f) {
                                      if (features[5] < 254.500000f) {
                                          return 2;
                                      } else {
                                          if (features[3] < 120.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 254.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          } else {
                              if (features[0] < 1087.500000f) {
                                  if (features[3] < 130.500000f) {
                                      if (features[7] < 130.500000f) {
                                          if (features[2] < 30.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 188.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 136.500000f) {
                                          if (features[6] < 82.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 145.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[3] < 130.500000f) {
                              if (features[5] < 254.500000f) {
                                  if (features[7] < 254.500000f) {
                                      return 2;
                                  } else {
                                      if (features[5] < 134.500000f) {
                                          if (features[2] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[4] < 98.500000f) {
                                      if (features[3] < 61.500000f) {
                                          if (features[4] < 57.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 87.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[3] < 132.500000f) {
                                  if (features[4] < 1.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 144.500000f) {
                                      if (features[4] < 0.500000f) {
                                          if (features[5] < 228.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[4] < 0.500000f) {
                                          if (features[5] < 252.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[7] < 36.500000f) {
                          if (features[5] < 254.500000f) {
                              if (features[5] < 128.500000f) {
                                  if (features[4] < 45.500000f) {
                                      if (features[3] < 4.500000f) {
                                          if (features[3] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[3] < 42.500000f) {
                                  if (features[4] < 34.500000f) {
                                      return 2;
                                  } else {
                                      if (features[7] < 35.500000f) {
                                          return 2;
                                      } else {
                                          return 4;
                                      }
                                  }
                              } else {
                                  if (features[0] < 1086.500000f) {
                                      return 2;
                                  } else {
                                      if (features[3] < 69.500000f) {
                                          if (features[3] < 65.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[0] < 304.500000f) {
                              if (features[0] < 303.500000f) {
                                  return 2;
                              } else {
                                  if (features[3] < 104.500000f) {
                                      return 2;
                                  } else {
                                      if (features[5] < 245.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          } else {
                              if (features[3] < 69.500000f) {
                                  if (features[3] < 68.500000f) {
                                      if (features[7] < 254.500000f) {
                                          if (features[3] < 4.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 96.500000f) {
                                          if (features[2] < 204.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_2(const float *features, int32_t features_length) {
          if (features[5] < 143.500000f) {
              if (features[2] < 0.500000f) {
                  if (features[7] < 0.500000f) {
                      if (features[1] < 6.500000f) {
                          return 0;
                      } else {
                          if (features[3] < 1.500000f) {
                              if (features[6] < 2.500000f) {
                                  if (features[5] < 0.500000f) {
                                      if (features[0] < 1.500000f) {
                                          return 1;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  return 0;
                              }
                          } else {
                              if (features[0] < 1382.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      }
                  } else {
                      if (features[5] < 129.500000f) {
                          if (features[0] < 1682.500000f) {
                              if (features[3] < 238.500000f) {
                                  if (features[0] < 321.500000f) {
                                      if (features[5] < 0.500000f) {
                                          if (features[6] < 75.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[0] < 495.500000f) {
                                          return 2;
                                      } else {
                                          if (features[7] < 132.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          return 2;
                      }
                  }
              } else {
                  if (features[0] < 1520.500000f) {
                      if (features[6] < 36.500000f) {
                          if (features[4] < 128.500000f) {
                              if (features[3] < 192.500000f) {
                                  if (features[7] < 152.500000f) {
                                      if (features[0] < 159.500000f) {
                                          return 2;
                                      } else {
                                          if (features[4] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 0.500000f) {
                                          if (features[0] < 402.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 32.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[0] < 687.500000f) {
                                      return 2;
                                  } else {
                                      if (features[7] < 0.500000f) {
                                          if (features[5] < 112.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[7] < 36.500000f) {
                                  if (features[7] < 28.500000f) {
                                      if (features[0] < 788.500000f) {
                                          return 2;
                                      } else {
                                          if (features[2] < 13.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 790.500000f) {
                                          if (features[5] < 24.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[0] < 160.500000f) {
                                      if (features[0] < 159.500000f) {
                                          return 2;
                                      } else {
                                          if (features[3] < 173.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 5.500000f) {
                                          if (features[6] < 33.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[7] < 2.500000f) {
                              if (features[7] < 0.500000f) {
                                  if (features[2] < 5.500000f) {
                                      if (features[5] < 111.500000f) {
                                          if (features[5] < 4.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 7.500000f) {
                                          if (features[0] < 1115.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[5] < 29.500000f) {
                                      if (features[0] < 666.500000f) {
                                          if (features[1] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 32.500000f) {
                                          if (features[0] < 1004.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[3] < 128.500000f) {
                                  if (features[5] < 48.500000f) {
                                      if (features[6] < 254.500000f) {
                                          if (features[0] < 790.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 138.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 254.500000f) {
                                          return 2;
                                      } else {
                                          if (features[3] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[3] < 155.500000f) {
                                      if (features[5] < 48.500000f) {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 48.500000f) {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[3] < 0.500000f) {
                          if (features[7] < 0.500000f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[2] < 3.500000f) {
                              if (features[4] < 18.500000f) {
                                  if (features[2] < 2.500000f) {
                                      return 2;
                                  } else {
                                      if (features[0] < 1994.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 140.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[5] < 120.500000f) {
                                      if (features[3] < 127.500000f) {
                                          if (features[3] < 126.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[6] < 252.500000f) {
                                          if (features[4] < 40.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          } else {
                              if (features[4] < 2.500000f) {
                                  if (features[7] < 0.500000f) {
                                      if (features[5] < 44.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[4] < 16.500000f) {
                  if (features[0] < 339.500000f) {
                      if (features[5] < 254.500000f) {
                          return 2;
                      } else {
                          if (features[6] < 64.500000f) {
                              if (features[2] < 119.500000f) {
                                  return 0;
                              } else {
                                  if (features[7] < 77.500000f) {
                                      return 2;
                                  } else {
                                      if (features[4] < 11.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[3] < 102.500000f) {
                                  return 2;
                              } else {
                                  if (features[7] < 148.500000f) {
                                      if (features[0] < 230.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[7] < 15.500000f) {
                          if (features[5] < 209.500000f) {
                              if (features[3] < 40.500000f) {
                                  if (features[0] < 1376.500000f) {
                                      if (features[0] < 487.500000f) {
                                          return 2;
                                      } else {
                                          if (features[5] < 178.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[4] < 0.500000f) {
                                      if (features[7] < 1.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[4] < 7.500000f) {
                                          if (features[7] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[7] < 0.500000f) {
                                  if (features[6] < 27.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[0] < 498.500000f) {
                                      if (features[0] < 495.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[3] < 0.500000f) {
                              if (features[0] < 1938.500000f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          } else {
                              if (features[7] < 16.500000f) {
                                  if (features[2] < 0.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[7] < 253.500000f) {
                                      return 2;
                                  } else {
                                      if (features[4] < 0.500000f) {
                                          if (features[2] < 14.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  }
              } else {
                  if (features[2] < 16.500000f) {
                      if (features[2] < 0.500000f) {
                          if (features[5] < 254.500000f) {
                              if (features[7] < 13.500000f) {
                                  if (features[5] < 236.500000f) {
                                      if (features[0] < 1244.500000f) {
                                          if (features[7] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[4] < 27.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              } else {
                                  if (features[0] < 500.500000f) {
                                      if (features[6] < 25.500000f) {
                                          if (features[0] < 468.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[4] < 106.500000f) {
                                  if (features[4] < 81.500000f) {
                                      if (features[3] < 89.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[6] < 119.500000f) {
                              if (features[5] < 254.500000f) {
                                  if (features[4] < 86.500000f) {
                                      if (features[3] < 10.500000f) {
                                          if (features[5] < 214.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1200.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 37.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[0] < 922.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 106.500000f) {
                                              return 0;
                                          } else {
                                              return 3;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 87.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 946.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 126.500000f) {
                                  if (features[3] < 65.500000f) {
                                      if (features[4] < 96.500000f) {
                                          if (features[3] < 61.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  } else {
                      if (features[5] < 254.500000f) {
                          if (features[7] < 0.500000f) {
                              if (features[0] < 1196.500000f) {
                                  return 2;
                              } else {
                                  if (features[4] < 37.500000f) {
                                      if (features[5] < 144.500000f) {
                                          if (features[2] < 185.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 9.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 1202.500000f) {
                                          if (features[3] < 34.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[0] < 792.500000f) {
                              if (features[0] < 789.500000f) {
                                  return 2;
                              } else {
                                  return 4;
                              }
                          } else {
                              if (features[2] < 65.500000f) {
                                  if (features[6] < 252.500000f) {
                                      return 2;
                                  } else {
                                      if (features[4] < 66.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[2] < 204.500000f) {
                                      if (features[6] < 104.500000f) {
                                          if (features[4] < 96.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_3(const float *features, int32_t features_length) {
          if (features[0] < 0.500000f) {
              if (features[6] < 1.500000f) {
                  return 1;
              } else {
                  return 2;
              }
          } else {
              if (features[4] < 35.500000f) {
                  if (features[0] < 1680.500000f) {
                      if (features[4] < 0.500000f) {
                          if (features[0] < 1.500000f) {
                              return 2;
                          } else {
                              if (features[5] < 7.500000f) {
                                  if (features[6] < 78.500000f) {
                                      if (features[3] < 160.500000f) {
                                          if (features[6] < 18.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 5.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 241.500000f) {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  }
                              } else {
                                  if (features[0] < 1353.500000f) {
                                      if (features[0] < 302.500000f) {
                                          if (features[6] < 20.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 124.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[6] < 8.500000f) {
                              if (features[0] < 1351.500000f) {
                                  if (features[1] < 6.500000f) {
                                      return 0;
                                  } else {
                                      if (features[7] < 140.500000f) {
                                          if (features[6] < 2.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 36.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[2] < 27.500000f) {
                                  if (features[3] < 54.500000f) {
                                      if (features[2] < 23.500000f) {
                                          if (features[5] < 105.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 689.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 791.500000f) {
                                          if (features[5] < 254.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[2] < 254.500000f) {
                                      if (features[5] < 7.500000f) {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 19.500000f) {
                                          if (features[6] < 96.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[1] < 6.500000f) {
                          return 0;
                      } else {
                          if (features[0] < 1998.500000f) {
                              if (features[2] < 0.500000f) {
                                  if (features[6] < 0.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 126.500000f) {
                                      return 2;
                                  } else {
                                      if (features[5] < 127.500000f) {
                                          return 2;
                                      } else {
                                          if (features[5] < 128.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 254.500000f) {
                                  if (features[6] < 0.500000f) {
                                      if (features[2] < 149.500000f) {
                                          if (features[7] < 70.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[3] < 0.500000f) {
                                          if (features[5] < 167.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 169.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[0] < 2016.500000f) {
                                      if (features[0] < 2001.500000f) {
                                          return 0;
                                      } else {
                                          if (features[0] < 2007.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              } else {
                  if (features[3] < 40.500000f) {
                      if (features[2] < 100.500000f) {
                          if (features[4] < 252.500000f) {
                              if (features[5] < 105.500000f) {
                                  if (features[7] < 151.500000f) {
                                      if (features[6] < 151.500000f) {
                                          if (features[3] < 31.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 19.500000f) {
                                          if (features[0] < 609.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 46.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[0] < 497.500000f) {
                                      if (features[2] < 0.500000f) {
                                          if (features[7] < 228.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[0] < 1201.500000f) {
                                          if (features[3] < 9.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[0] < 513.500000f) {
                                  if (features[7] < 250.500000f) {
                                      if (features[0] < 488.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      if (features[0] < 369.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              } else {
                                  if (features[5] < 237.500000f) {
                                      if (features[2] < 5.500000f) {
                                          if (features[3] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 238.500000f) {
                                          return 0;
                                      } else {
                                          if (features[7] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[7] < 0.500000f) {
                              if (features[0] < 1209.500000f) {
                                  if (features[0] < 670.500000f) {
                                      return 2;
                                  } else {
                                      if (features[6] < 10.500000f) {
                                          return 0;
                                      } else {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[6] < 254.500000f) {
                                  if (features[5] < 0.500000f) {
                                      if (features[1] < 7.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[4] < 49.500000f) {
                                      if (features[2] < 239.500000f) {
                                          if (features[2] < 174.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[4] < 36.500000f) {
                          if (features[3] < 41.500000f) {
                              if (features[7] < 78.500000f) {
                                  if (features[7] < 35.500000f) {
                                      return 0;
                                  } else {
                                      return 4;
                                  }
                              } else {
                                  if (features[6] < 22.500000f) {
                                      return 0;
                                  } else {
                                      if (features[5] < 50.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 3.500000f) {
                                  if (features[7] < 44.500000f) {
                                      if (features[5] < 55.500000f) {
                                          return 0;
                                      } else {
                                          if (features[5] < 60.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[7] < 39.500000f) {
                                      if (features[0] < 809.500000f) {
                                          if (features[2] < 118.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[6] < 254.500000f) {
                                          if (features[5] < 29.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[2] < 1.500000f) {
                              if (features[4] < 96.500000f) {
                                  if (features[7] < 0.500000f) {
                                      if (features[5] < 246.500000f) {
                                          return 0;
                                      } else {
                                          if (features[6] < 106.500000f) {
                                              return 0;
                                          } else {
                                              return 3;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 1088.500000f) {
                                          if (features[3] < 239.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[6] < 25.500000f) {
                                      if (features[2] < 0.500000f) {
                                          if (features[0] < 525.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1199.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 1201.500000f) {
                                  if (features[6] < 126.500000f) {
                                      if (features[6] < 18.500000f) {
                                          if (features[0] < 159.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 71.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 48.500000f) {
                                          if (features[2] < 78.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[0] < 1442.500000f) {
                                      if (features[0] < 1441.500000f) {
                                          return 2;
                                      } else {
                                          if (features[5] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[7] < 254.500000f) {
                                          if (features[2] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 179.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_4(const float *features, int32_t features_length) {
          if (features[4] < 35.500000f) {
              if (features[4] < 0.500000f) {
                  if (features[7] < 0.500000f) {
                      if (features[2] < 0.500000f) {
                          if (features[3] < 1.500000f) {
                              if (features[0] < 1.500000f) {
                                  return 1;
                              } else {
                                  return 0;
                              }
                          } else {
                              return 0;
                          }
                      } else {
                          if (features[5] < 157.500000f) {
                              if (features[3] < 190.500000f) {
                                  if (features[0] < 1606.500000f) {
                                      return 0;
                                  } else {
                                      if (features[2] < 74.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[3] < 191.500000f) {
                                      if (features[2] < 210.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 0;
                                  }
                              }
                          } else {
                              if (features[0] < 685.500000f) {
                                  return 2;
                              } else {
                                  if (features[0] < 1399.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[3] < 145.500000f) {
                          if (features[0] < 1682.500000f) {
                              if (features[5] < 0.500000f) {
                                  if (features[0] < 1271.500000f) {
                                      if (features[6] < 18.500000f) {
                                          return 0;
                                      } else {
                                          if (features[6] < 19.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 21.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[0] < 1269.500000f) {
                                      if (features[2] < 35.500000f) {
                                          if (features[7] < 17.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[2] < 0.500000f) {
                              if (features[3] < 228.500000f) {
                                  if (features[7] < 179.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[7] < 121.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 790.500000f) {
                                  if (features[0] < 786.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              } else {
                  if (features[0] < 1088.500000f) {
                      if (features[3] < 40.500000f) {
                          if (features[0] < 338.500000f) {
                              if (features[2] < 229.500000f) {
                                  return 2;
                              } else {
                                  if (features[2] < 230.500000f) {
                                      if (features[7] < 54.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[6] < 8.500000f) {
                                  if (features[4] < 6.500000f) {
                                      if (features[7] < 33.500000f) {
                                          if (features[3] < 27.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[7] < 140.500000f) {
                                          if (features[7] < 35.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 41.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[5] < 105.500000f) {
                                      if (features[0] < 607.500000f) {
                                          if (features[5] < 16.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 31.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 129.500000f) {
                                          if (features[3] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[7] < 35.500000f) {
                              if (features[5] < 33.500000f) {
                                  if (features[5] < 6.500000f) {
                                      if (features[6] < 18.500000f) {
                                          if (features[0] < 342.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[1] < 6.500000f) {
                                          return 0;
                                      } else {
                                          if (features[7] < 15.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[5] < 241.500000f) {
                                      if (features[2] < 0.500000f) {
                                          if (features[3] < 151.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 19.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[5] < 254.500000f) {
                                  if (features[6] < 254.500000f) {
                                      if (features[0] < 160.500000f) {
                                          if (features[0] < 159.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 12.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 125.500000f) {
                                          if (features[5] < 53.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 136.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[2] < 38.500000f) {
                                      if (features[7] < 132.500000f) {
                                          if (features[2] < 10.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[4] < 8.500000f) {
                                          if (features[0] < 388.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[3] < 0.500000f) {
                          if (features[5] < 0.500000f) {
                              return 0;
                          } else {
                              if (features[6] < 1.500000f) {
                                  if (features[2] < 187.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  if (features[0] < 2024.500000f) {
                                      return 2;
                                  } else {
                                      if (features[5] < 167.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[7] < 0.500000f) {
                              if (features[0] < 1360.500000f) {
                                  if (features[2] < 238.500000f) {
                                      if (features[6] < 17.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 242.500000f) {
                                          if (features[3] < 80.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 58.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 4.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[4] < 16.500000f) {
                                  if (features[0] < 1998.500000f) {
                                      if (features[4] < 1.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[3] < 126.500000f) {
                                          return 2;
                                      } else {
                                          if (features[7] < 252.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[2] < 48.500000f) {
                                      if (features[7] < 254.500000f) {
                                          return 2;
                                      } else {
                                          if (features[3] < 128.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[2] < 68.500000f) {
                  if (features[7] < 0.500000f) {
                      if (features[5] < 254.500000f) {
                          if (features[3] < 49.500000f) {
                              if (features[2] < 45.500000f) {
                                  if (features[3] < 40.500000f) {
                                      if (features[0] < 1507.500000f) {
                                          if (features[0] < 276.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[6] < 11.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[5] < 227.500000f) {
                                      if (features[5] < 83.500000f) {
                                          if (features[6] < 192.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 111.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[4] < 228.500000f) {
                                  if (features[0] < 669.500000f) {
                                      return 2;
                                  } else {
                                      if (features[6] < 0.500000f) {
                                          return 0;
                                      } else {
                                          if (features[0] < 690.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 1.500000f) {
                                      return 0;
                                  } else {
                                      if (features[2] < 3.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[0] < 695.500000f) {
                              if (features[0] < 192.500000f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          } else {
                              if (features[3] < 66.500000f) {
                                  return 0;
                              } else {
                                  if (features[6] < 106.500000f) {
                                      return 0;
                                  } else {
                                      return 3;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[7] < 35.500000f) {
                          if (features[6] < 155.500000f) {
                              if (features[4] < 156.500000f) {
                                  if (features[6] < 151.500000f) {
                                      if (features[0] < 1087.500000f) {
                                          if (features[4] < 95.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 34.500000f) {
                                          if (features[2] < 52.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[2] < 5.500000f) {
                                      if (features[4] < 159.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 815.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 32.500000f) {
                                          if (features[7] < 17.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 33.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 254.500000f) {
                                  if (features[6] < 157.500000f) {
                                      if (features[5] < 35.500000f) {
                                          if (features[0] < 735.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 10.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[0] < 1088.500000f) {
                              if (features[0] < 496.500000f) {
                                  if (features[5] < 254.500000f) {
                                      if (features[4] < 227.500000f) {
                                          if (features[5] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 36.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 2.500000f) {
                                          if (features[4] < 82.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[6] < 126.500000f) {
                                      if (features[5] < 254.500000f) {
                                          if (features[6] < 21.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 87.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 121.500000f) {
                                          if (features[7] < 141.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[7] < 254.500000f) {
                                  if (features[0] < 2024.500000f) {
                                      return 2;
                                  } else {
                                      if (features[0] < 2025.500000f) {
                                          if (features[5] < 29.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[0] < 2003.500000f) {
                                      if (features[0] < 1543.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 190.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          }
                      }
                  }
              } else {
                  if (features[5] < 254.500000f) {
                      if (features[5] < 29.500000f) {
                          if (features[2] < 220.500000f) {
                              if (features[0] < 809.500000f) {
                                  if (features[5] < 6.500000f) {
                                      return 2;
                                  } else {
                                      if (features[6] < 151.500000f) {
                                          if (features[7] < 64.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 62.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[0] < 1224.500000f) {
                                          if (features[6] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[6] < 254.500000f) {
                                          return 2;
                                      } else {
                                          if (features[3] < 8.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[3] < 5.500000f) {
                                  if (features[0] < 1228.500000f) {
                                      if (features[2] < 254.500000f) {
                                          if (features[0] < 684.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 126.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[5] < 7.500000f) {
                                      if (features[6] < 0.500000f) {
                                          if (features[7] < 9.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 6.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 173.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 130.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[7] < 0.500000f) {
                              if (features[6] < 1.500000f) {
                                  if (features[5] < 221.500000f) {
                                      return 0;
                                  } else {
                                      if (features[0] < 978.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[2] < 75.500000f) {
                                  if (features[6] < 254.500000f) {
                                      if (features[5] < 33.500000f) {
                                          if (features[6] < 47.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[4] < 187.500000f) {
                                          if (features[0] < 1102.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[3] < 149.500000f) {
                                      if (features[6] < 254.500000f) {
                                          if (features[7] < 94.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 48.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 153.500000f) {
                                          if (features[5] < 48.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[2] < 69.500000f) {
                          if (features[4] < 66.500000f) {
                              return 4;
                          } else {
                              if (features[3] < 79.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[0] < 1085.500000f) {
                              return 2;
                          } else {
                              if (features[4] < 96.500000f) {
                                  if (features[0] < 1087.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_PUBLIC int32_t can_ids_predict(const float *features, int32_t features_length) {

        int32_t votes[5] = {0,};
        int32_t _class = -1;

        _class = can_ids_tree_0(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_1(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_2(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_3(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_4(features, features_length); votes[_class] += 1;
    
        int32_t most_voted_class = -1;
        int32_t most_voted_votes = 0;
        for (int32_t i=0; i<5; i++) {

            if (votes[i] > most_voted_votes) {
                most_voted_class = i;
                most_voted_votes = votes[i];
            }
        }
        return most_voted_class;
    }
    

int can_ids_predict_proba(const float *features, int32_t features_length, float *out, int out_length) {

        int32_t _class = -1;

        for (int i=0; i<out_length; i++) {
            out[i] = 0.0f;
        }

        _class = can_ids_tree_0(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_1(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_2(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_3(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_4(features, features_length); out[_class] += 1.0f;
    
        // compute mean
        for (int i=0; i<out_length; i++) {
            out[i] = out[i] / 5;
        }
        return 0;
    }
    
// Which model this is: shown by the firmware in READY and #STAT.
#define CAN_IDS_MODEL_ID "rf5-d9-p6-e7321e76"

// CAN IDs in the normal training traffic (38 of 2048), one bit per ID.
static const uint8_t can_ids_known[256] = {
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x46, 0x00, 0x03, 0x00, 0x00
};

CAN_IDS_FN int can_ids_id_known(uint32_t can_id)
{
    return (can_ids_known[(can_id >> 3) & 0xFF] >> (can_id & 7)) & 1;
}

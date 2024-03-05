import matplotlib.pyplot as plt
import numpy as np
import laterationFunctions as lf


def knownNValues(known_points, rxRssiDic, limit = 2, show = 0):

    knownDistances = {}

    for i, point in enumerate(known_points):
        knownDistances[i] = []

        for j in range(len(known_points)):
            if i == j:
                knownDistances[i].append(0)
            else:
                knownDistances[i].append(round(lf.distance(point, known_points[j]), 2))

    nDic = {}
    for key in rxRssiDic.keys():
        nDic[key] = {}
        for i in range(len(rxRssiDic[key])):
            nDic[key][i] = []
            for j in range(4):
                if i!=j:
                    nDic[key][i].append(round(lf.estimateN(knownDistances[i][j], rxRssiDic[key][i][j]),2))
                else:
                    nDic[key][i].append(0)

    # 0 1 
    # 0 2
    # 0 3
    # 1 2 
    # 1 3 
    # 2 3

    possibilities = [(0,1), (0,2), (0,3), (1,2),(1,3),(2,3)]

    nDicS = {}
    for key in nDic.keys():
        for i in range(len(nDic[key].keys())):
            for j in range(len(nDic[key].keys())):
                if (i,j) in possibilities:
                    if (i,j) not in nDicS.keys():
                        nDicS[(i,j)] = [nDic[key][i][j]]
                    else:
                        nDicS[(i,j)].append(nDic[key][i][j])
    overallN = []


    for key in nDicS.keys():
       
        # print(key, (nDicS[key]))
        coordsDiff = []
        for i in range(3):
            coordsDiff.append(abs(round(known_points[key[0]][i] - known_points[key[1]][i],2)))

        for nVal in nDicS[key]:
            overallN.append(nVal)
           
        # print(f"Rx{key[0]}: {known_points[key[0]]} | Rx{key[1]}: {known_points[key[1]]}\n")
        if show:
            print(key, sorted(nDicS[key]))
            print(f"Mean: {round(np.mean(nDicS[key]),2)} | Median: {np.median(nDicS[key])} | SD: {round(np.std(nDicS[key]),2)}")
            print(f"Diff = {coordsDiff}\n") 

            print(f"Overall | Mean: {round(np.mean(overallN),2)} | Median: {np.median(overallN)} | SD: {round(np.std(overallN),2)}|")


            print(calculateMean(nDicS))
    return nDicS, calculateMean(nDicS, limit = limit)



def weightedN(nDicS, rx, limit = 3):
    total = 0
    n = 0
    for key in nDicS.keys():
        if rx in key:
            if nDicS[key][0]>limit:

                total+= nDicS[key][0]
                n+=1
    if n>0:
        
        return total/n
    else:
        return 3.5

def calculateMean(nDicS, limit = 3):
    total = 0
    n = 0
    for key in nDicS:
        for val in nDicS[key]:
            if val > limit:

                total +=val
                n+=1
    if n != 0:
        return total/n
    else:
        return 3.5

import tensorflow as tf
import struct
import numpy
from sklearn.utils import shuffle
import subprocess
import os
import gc
import random
import os.path
from pathlib import Path
from tensorflow import keras
import chess_games


class one:
    def __init__(self,shape):
        self.shape = shape
    def __index__(self,x):
        return 1



num_layers = 3
layerTypes = ['d','d','d','d','d']
layer_types = ['d','d','d','d','d']
layer_sizes = [64,64, 1]

times_input_repeated = 1
             #pieces      #side_to_move   #en_passant_squares   #castling_rights
input_size = (64 * 2 * 6 + 4);

weights_filename = "chess.nnue"


def load_weights(model):
    trained_weights = []
    weights_file = open(weights_filename,"rb+")
    for i in range(0,num_layers*2,2):
        
        layer_index = i//2
        layer_type =layer_types[layer_index]
        print(layer_index)
        weights = []
        biases = []
        #print(weights.shape)

        if layer_type == "c":
            count = 1
            for i in layer_sizes[layer_index]:
                count *= i
            print(count)
            curr_layer_weights = numpy.fromfile(weights_file, count = count, dtype=numpy.float32)
            curr_layer_weights.resize(layer_sizes[layer_index])
            
            trained_weights.append(curr_layer_weights)
            curr_layer_biases = numpy.fromfile(weights_file, count = layer_sizes[layer_index][-1], dtype=numpy.float32)
            trained_weights.append(curr_layer_biases)
        elif layer_type == "d":
            if layer_index == 0:
                prev_layer_size = input_size
            elif layer_types[layer_index-1] == "c":
                prev_layer_size = 8*8*layer_sizes[layer_index-1][-1]
            else:
                prev_layer_size = layer_sizes[layer_index-1]
            print(prev_layer_size)  
            curr_layer_weights = numpy.fromfile(weights_file, count = prev_layer_size * layer_sizes[layer_index], dtype=numpy.float32)
            curr_layer_weights.resize((prev_layer_size,layer_sizes[layer_index]))
            trained_weights.append(curr_layer_weights)
            curr_layer_biases = numpy.fromfile(weights_file, count = layer_sizes[layer_index], dtype=numpy.float32)
            trained_weights.append(curr_layer_biases)

    weights_file.close()

    model.set_weights(trained_weights)

def save_weights(model):
    print("saving to file don't close")
    model_weights = model.get_weights()
    weights_file = open(weights_filename,"wb+")
    for i in range(0,num_layers*2,2):
        print(i)
        layer_index = i//2
        layer_type =layer_types[layer_index]

        weights = model_weights[i]
        biases = model_weights[i+1]
        print(weights.shape)
        if layer_type == "c":
            for filter_row in range(layer_sizes[layer_index][0]):
                for filter_col in range(layer_sizes[layer_index][1]):
                    for input_channel in range(layer_sizes[layer_index][2]):
                        for output_channel in range(layer_sizes[layer_index][3]):
                            weights_file.write(struct.pack('f', weights[filter_row][filter_col][input_channel][output_channel]))
            for bias in biases:
                weights_file.write(struct.pack('f', bias))
        elif layer_type == "d":
            for row in weights:
                for col in row:
                    weights_file.write(struct.pack('f', col))
                    #weights_file.write(struct.pack('f', weights[row][col]))
            for bias in biases:
                weights_file.write(struct.pack('f', bias))
    weights_file.close()
    print(model_weights)
    print("saved to file can close now")


class GCAdam(tf.keras.optimizers.Adam):
    def get_gradients(self, loss, params):
        # We here just provide a modified get_gradients() function since we are
        # trying to just compute the centralized gradients.

        grads = []
        gradients = super().get_gradients()
        for grad in gradients:
            grad_len = len(grad.shape)
            if grad_len > 1:
                axis = list(range(grad_len - 1))
                grad -= tf.reduce_mean(grad, axis=axis, keep_dims=True)
            grads.append(grad)

        return grads

    

model = tf.keras.Sequential()
model.add(tf.keras.layers.Input(shape=(input_size), sparse = True))
model.add(tf.keras.layers.Dense(layer_sizes[0], activation = 'relu'))
#model.add(keras.layers.Dropout(1/2))
model.add(tf.keras.layers.Dense(layer_sizes[1], activation = 'tanh'))
#model.add(keras.layers.Dropout(1/32))
#model.add(tf.keras.layers.Dense(layerSizes[2], activation = 'relu'))
#model.add(keras.layers.Dropout(0.03))
model.add(tf.keras.layers.Dense(layer_sizes[-1], activation = 'tanh'))

initial_learning_rate = 0.0002
lr = initial_learning_rate
opt = GCAdam(learning_rate=lr)
model.compile(optimizer=opt, loss='mean_squared_error')
load_weights(model)


layer_outputs = []

#for i in range(1, len(model.layers)):
    #tmp_model = keras.Model(model.layers[0].input, model.layers[i].output)
    #tmp_output = tmp_model.predict(x[:1])[0]
    #layer_outputs.append(tmp_output)

#x = x[:100]

#print(model.predict(x))
#del x
#del y
gc.collect()


#model_weights = model.get_weights()



file_names = [
        "C:/Users/Anastazja/source/repos/nina-chess/nina-chess/games.bin"
    ]

num_files = 0

for filename in file_names:
    if Path(filename).exists():
        num_files+=1
print("detected",num_files,"files")
prev_loss = 100000000
#subprocess.call("nina-chess.exe")
current_file = 0
attempts = 0
generation = 0;
lr_drops = 0
loss_sample_size = 1
batch_size_multipliers = [1,128]
curr_batch_size_multiplier = batch_size_multipliers[0]
epoch = 0
#subprocess.call("nina-chess.exe")
while(True):
        #save_weights(model)
        
        gc.collect()
        curr_loss = 0
        loss_sample_size = 0
        while True:
            games = chess_games.get_games()
            if(games is None):
                break
            loss_sample_size +=1
            x_val_raw,y_val = games
            np_ones = tf.ones((x_val_raw.shape[0]))
            x_val = tf.SparseTensor(x_val_raw,np_ones,(x_val_raw[-1,0]+1,input_size))
            x_pred = model.predict(tf.sparse.slice(x_val,[0,0],[100,input_size]))
            for i in range(100):
                    print(x_pred[i], y_val[i])
            curr_loss += model.evaluate(x_val, y_val,batch_size = 256 * 1)
        curr_loss /=loss_sample_size
        
            
        del x_val_raw
        del x_val
        del y_val
        gc.collect()
        
        if((prev_loss > curr_loss)):
            print("new best loss", curr_loss)
            prev_loss = curr_loss
            save_weights(model)
        else:
            print("did great, lowering learning rate", curr_loss, prev_loss)
            load_weights(model)
            if(curr_batch_size_multiplier == batch_size_multipliers[0]):
                curr_batch_size_multiplier = batch_size_multipliers[-1]
            else:
                curr_batch_size_multiplier = batch_size_multipliers[0]
                lr *= 0.1
                opt.learning_rate.assign(lr)
                attempts +=1
                if(attempts ==2):
                    print("ended")
                    exit(0)
                    attempts = 0
                    prev_loss = 10000000
                    opt.learning_rate.assign(initial_learning_rate)
                    lr = initial_learning_rate
                    epoch = 0
                    #os.remove("games.bin")
                    #os.remove("valdata.bin")
                    #subprocess.call("nina-chess.exe")
        gc.collect()
        index = 0
        while(True):
                print("\nattempt nr",attempts," epoch nr",epoch,"\n")
                games = chess_games.get_games()
                if(games is None):
                    current_file+=1
                    if(current_file >=num_files):
                        #subprocess.call("nina-chess.exe")
                        current_file=0
                        break
                    continue
                x_raw,y = games
                np_ones = tf.ones((x_raw.shape[0]))
                x = tf.SparseTensor(x_raw,np_ones,(x_raw[-1,0]+1,input_size))
                model.fit(x,y, epochs = 1
                      ,shuffle=True,batch_size = 256 * curr_batch_size_multiplier, verbose = 1)
                del x
                del y
                del x_raw
                del games
                index+=0
                gc.collect()
        epoch +=1
            #save_weights(model)
        #lr *= 0.1
        #opt.learning_rate.assign(lr)
        #lr_drops+=1
        #if(lr_drops >= 3):
        #    subprocess.call("nina-chess.exe")
        #    lr_drops = 0
         #   lr = initial_learning_rate
         #   opt.learning_rate.assign(lr)
        
        #




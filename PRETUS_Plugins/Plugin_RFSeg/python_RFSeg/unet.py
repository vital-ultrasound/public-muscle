import torch
import torch.nn as nn
import numpy as np

class UNet(nn.Module):

    def __init__(self, input_size, kernel_size=(3, 3, 3, 3, 3), output_channels=(8, 16, 32, 64, 128), strides=(2, 2, 2, 2, 2)):
        """
        Linear network that takes each scan line in the input and produces one value (at the end of the scan line)
        in the output. This is achieved by an output linear layer with 1 feature in the output.
        Args:
            input_size:  shape of the input image. Should be a 2 element vector for a 2D image.
        """

        super().__init__()

        self.name = 'UNet'
        # This should be in spherical coordinates
        self.input_size = input_size
        self.n_output_channels = (1, ) + output_channels # add the channels of the input image
        self.kernel_size = kernel_size
        self.strides = strides

        # compute paddings
        self.layer_sizes = (list(input_size),)
        for idx in range(len(self.n_output_channels) - 1):
            newsize = [
                np.floor((s + 2 * np.floor((self.kernel_size[idx] - 1) / 2) - self.kernel_size[idx]) / self.strides[idx] + 1).astype(np.int)
                for s in self.layer_sizes[idx]]
            self.layer_sizes = self.layer_sizes + (newsize,)

        # layer_size = (np.array(input_size[1:]),)  # todo hardcoded for dim = 0. Other dimensions need to be re-implemented
        # for i in range(len(self.kernel)-1):
        #     layer_size += (np.array(np.ceil(layer_size[i]), dtype=np.int),)

        # define the "encoder". First a few convs and then layers are: strided conv - conv cond

        self.input_layer = nn.Sequential(
            nn.Conv2d(in_channels=self.n_output_channels[0], out_channels=self.n_output_channels[1],
                      kernel_size=self.kernel_size[0], stride=1, padding=1),
            nn.ReLU(),
            nn.BatchNorm2d(self.n_output_channels[1]),
            #
            nn.Conv2d(in_channels=self.n_output_channels[1], out_channels=self.n_output_channels[1],
                          kernel_size=self.kernel_size[1], stride=1, padding=1),
            nn.ReLU(),
            nn.BatchNorm2d(self.n_output_channels[1]),
        )
        # encoder downsample -> conv
        layers = []
        for i in range(1,len(self.kernel_size)):
            current_layer = nn.Sequential(

                nn.Conv2d(in_channels=self.n_output_channels[i], out_channels=self.n_output_channels[i],
                          kernel_size=self.kernel_size[i], stride=self.strides[i], padding=1),
                nn.ReLU(),
                nn.BatchNorm2d(self.n_output_channels[i]),
                #
                nn.Conv2d(in_channels=self.n_output_channels[i], out_channels=self.n_output_channels[i + 1],
                          kernel_size=self.kernel_size[i], stride=1, padding=1),
                nn.ReLU(),
                nn.BatchNorm2d(self.n_output_channels[i + 1]),
            )
            layers.append(current_layer)
        self.encoder = nn.Sequential(*layers)

        # bridge: upsample
        p = [s - ((self.layer_sizes[-1][idx + 1] - 1) * self.strides[-1] - 2 * np.floor((self.kernel_size[-1] - 1) / 2).astype(np.int) + self.kernel_size[-1])
             for idx, s in enumerate(self.layer_sizes[len(self.kernel_size)-1][1:])]
        self.bridge = nn.Sequential(
            nn.ConvTranspose2d(in_channels=self.n_output_channels[-1],
                               out_channels=self.n_output_channels[-2],
                               kernel_size=self.kernel_size[-1], stride=self.strides[-1], padding=1,
                               output_padding=p),
            nn.ReLU(),
            nn.BatchNorm2d(self.n_output_channels[-2]),
        )


        # decoder: goes conv *2, conv,  upsample,
        layers = []
        for i in reversed(range(len(self.kernel_size)-1)):
            p = [s - ((self.layer_sizes[i + 1][idx+1] - 1) * self.strides[i] - 2 * np.floor(
                (self.kernel_size[i] - 1) / 2).astype(np.int) + self.kernel_size[i])
                 for idx, s in enumerate(self.layer_sizes[i][1:])]
            if i == 0:
                upsampler = nn.Conv2d(in_channels=self.n_output_channels[i],
                                               out_channels=self.n_output_channels[i],
                                               kernel_size=self.kernel_size[i], stride=1, padding=1)
            else:
                upsampler = nn.ConvTranspose2d(in_channels=self.n_output_channels[i],
                                           out_channels=self.n_output_channels[i],
                                           kernel_size=self.kernel_size[i], stride=self.strides[i], padding=1,
                                           output_padding=p)

            current_layer = nn.Sequential(
                nn.Conv2d(in_channels=self.n_output_channels[i + 1] * 2, out_channels=self.n_output_channels[i+1],
                          kernel_size=self.kernel_size[i], stride=1, padding=1),
                nn.ReLU(),
                nn.BatchNorm2d(self.n_output_channels[i+1]),
                # standard conv
                nn.Conv2d(in_channels=self.n_output_channels[i + 1], out_channels=self.n_output_channels[i],
                          kernel_size=self.kernel_size[i], stride=1, padding=1),
                nn.ReLU(),
                nn.BatchNorm2d(self.n_output_channels[i]),
                # upsample and decrease n channels
                upsampler,
                nn.ReLU(),
                nn.BatchNorm2d(self.n_output_channels[i]),
            )
            layers.append(current_layer)
        self.decoder = nn.Sequential(*layers)
        # output layer
        self.output_layer = nn.Sequential(
            nn.Conv2d(in_channels=self.n_output_channels[0], out_channels=self.n_output_channels[0],
                           kernel_size=self.kernel_size[0], stride=1, padding=1),
            nn.Sigmoid(),
        )

        # default initialization
        self.apply(self.init_weights)

    def extra_repr(self):
        out_string = 'input_size={}'.format(self.input_size)
        return out_string

    def forward(self, data):
        feat = self.input_layer(data)
        features = [feat]
        for i, layer in enumerate(self.encoder):
            feat = layer(feat)
            if i < len(self.encoder)-1:
                features.append(feat)

        # bridge
        feat = self.bridge(feat)

        # decoder (summed skip connections)
        for id in reversed(range(len(self.decoder))):
            skip_i = features[id]
            #concat = feat + skip_i
            concat = torch.cat((feat, skip_i), dim=1)
            feat = self.decoder[len(self.decoder)-id-1](concat)

        y = self.output_layer(feat)
        return y

    def get_name(self):
        linear_feat_str = '_features{}'.format(self.linear_features).replace(', ', '_').replace('(', '').replace(')', '')
        return self.name + linear_feat_str

    def init_weights(self, m):
        # @todo investigate this
        if type(m) == nn.Conv2d:
            aa=1
            #m.apply(m.init_weights)
            #m.weight.datasets.fill_(0.0)
            #if m.bias is not None:
            #    #m.bias.datasets.uniform_(-0.1, 0.1)
            #    m.bias.datasets.fill_(0.0)

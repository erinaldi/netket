# Copyright 2021 The NetKet Authors - All rights reserved.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#    http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from typing import Union, Optional, Tuple, Any, Callable, Iterable

import numpy as np

import jax
from jax import numpy as jnp
from flax import linen as nn

from netket.hilbert import AbstractHilbert
from netket.graph import AbstractGraph

from netket import nn as nknn
from netket.nn.initializers import lecun_normal, variance_scaling, zeros, normal


PRNGKey = Any
Shape = Tuple[int]
Dtype = Any  # this could be a real type?
Array = Any

default_kernel_init = normal(stddev=0.1)


class RBM(nn.Module):
    """A restricted boltzman Machine, equivalent to a 2-layer FFNN with a
    nonlinear activation function in between

    Attributes:
        dtype: dtype of the weights.
        activation: The nonlinear activation function
        alpha: feature density. Number of features equal to alpha * input.shape[-1]
        use_bias: if True uses a bias in the dense layer (hidden layer bias)
        use_visible_bias: if True adds a bias to the input
        kernel_init: initializer function for the weight matrix.
        bias_init: initializer function for the bias.
        visible_bias_init: initializer function for the visible_bias.
    """

    dtype: Any = np.float64
    activation: Any = nknn.logcosh
    alpha: Union[float, int] = 1
    use_bias: bool = True
    use_visible_bias: bool = True

    kernel_init: Callable[[PRNGKey, Shape, Dtype], Array] = default_kernel_init
    bias_init: Callable[[PRNGKey, Shape, Dtype], Array] = default_kernel_init
    visible_bias_init: Callable[[PRNGKey, Shape, Dtype], Array] = default_kernel_init

    @nn.compact
    def __call__(self, input):
        x = nknn.Dense(
            name="Dense",
            features=int(self.alpha * input.shape[-1]),
            dtype=self.dtype,
            use_bias=self.use_bias,
            kernel_init=self.kernel_init,
            bias_init=self.bias_init,
        )(input)
        x = self.activation(x)
        x = jnp.sum(x, axis=-1)

        if self.use_visible_bias:
            v_bias = self.param(
                "visible_bias", self.visible_bias_init, (input.shape[-1],), self.dtype
            )
            out_bias = jnp.dot(input, v_bias)
            return x + out_bias
        else:
            return x


class RBMModPhase(nn.Module):
    """Two restricted boltzman machines, one encoding the real part and one
    encoding the imaginary part of the output

    Attributes:
        dtype: dtype of the weights.
        activation: The nonlinear activation function
        alpha: feature density. Number of features equal to alpha * input.shape[-1]
        use_bias: if True uses a bias in the dense layer (hidden layer bias)
        kernel_init: initializer function for the weight matrix.
        bias_init: initializer function for the bias.
    """

    dtype: Any = np.float64
    activation: Any = nknn.logcosh
    alpha: Union[float, int] = 1
    use_bias: bool = True

    kernel_init: Callable[[PRNGKey, Shape, Dtype], Array] = default_kernel_init
    bias_init: Callable[[PRNGKey, Shape, Dtype], Array] = default_kernel_init

    @nn.compact
    def __call__(self, x):
        re = nknn.Dense(
            features=int(self.alpha * x.shape[-1]),
            dtype=self.dtype,
            use_bias=self.use_bias,
            kernel_init=self.kernel_init,
            bias_init=self.bias_init,
        )(x)
        re = self.activation(re)
        re = jnp.sum(re, axis=-1)

        im = nknn.Dense(
            features=int(self.alpha * x.shape[-1]),
            dtype=self.dtype,
            use_bias=self.use_bias,
            kernel_init=self.kernel_init,
            bias_init=self.bias_init,
        )(x)
        im = self.activation(im)
        im = jnp.sum(im, axis=-1)

        return re + 1j * im
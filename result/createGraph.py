import matplotlib.pyplot as plt
import numpy as np
import csv


def generate_plot(
    x,
    y,
    im_name,
    width=26.5,
    height=14.4,
    figure_n=0,
    dpi=350.0,
    title='',
    y_title='',
    x_title='',
    y_min=None,
    y_max=None,
):
    """
    Function generates a plot of psd values. It then stores that plot in a png
    file
    Parameters
    ----------
    x : np.array
        array with x axis values
    y : np.array
        array with y axis values
    im_name : str
        name of the image
    width : float, optional
        width of the generated plot, by default 26.5
    height : float, optional
        height of the generated plot, by default 14.4
    figure_n : int, optional
        figure number of the plot, by default 0
    dpi : float, optional
        dpi of the stored plot image, by default 350.0
    title : str, optional
        title of the generated plot, by default ''
    y_title : str, optional
        title of the y axis of the plot, by default ''
    x_title : str, optional
        title of the x axis of the plot, by default ''
    y_min : float, optional
        minimum y value to show, by default None
    y_max : float, optional
        maximum y value to show on the plot, by default None
    """
    if not len(x) or not len(y) or not len(x) == len(y):
        return

    # generate the plot figure with correct dimensions
    plt.figure(num=figure_n, figsize=(width, height), dpi=dpi)
    plt.plot(x, y)

    # set the titles and limits
    axis = plt.gca()
    axis.set_ylim([y_min, y_max])
    plt.title(title)
    plt.xlabel(x_title)
    plt.ylabel(y_title)

    if len(x) > 10:
        step = int(len(x) / 10)
    else:
        step = 1

    # set the x axis labels
    plt.xticks([i for i in range(0, len(x), step)])

    # generate the plot image
    plt.savefig(f'{im_name}.png')
    plt.close(figure_n)
    print(im_name)


if __name__ == '__main__':
    x = []
    y = []
    
    with open('result.csv','r') as csvfile:
        plots = csv.reader(csvfile, delimiter = ',')

        print(plots)
        
        for row in plots:
            print(row)
            x.append(int(row[0]))
            y.append(int(row[2])) 
    
    generate_plot(x, y, 'beau plot')

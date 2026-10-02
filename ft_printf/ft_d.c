/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_d.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <akunegel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/23 19:00:24 by akunegel          #+#    #+#             */
/*   Updated: 2023/04/24 15:59:13 by akunegel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static int	ft_touille(int nb)
{
	int	i;

	i = 0;
	if (nb < 0)
		i++;
	if (!nb)
		return (1);
	while (nb)
	{
		i++;
		nb = nb / 10;
	}
	return (i);
}

int	ft_du(int nb)
{
	int	taille;

	taille = ft_touille(nb);
	if (nb == -2147483648)
	{
		write(1, "-2147483648", 11);
		return (11);
	}
	if (nb < 0)
	{
		write(1, "-", 1);
		nb = -nb;
	}
	if (nb > 9)
	{
		ft_du(nb / 10);
		ft_du(nb % 10);
	}
	if (nb < 10)
	{
		ft_putchar(nb + 48);
	}
	return (taille);
}

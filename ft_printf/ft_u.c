/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_u.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <akunegel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/23 19:33:24 by akunegel          #+#    #+#             */
/*   Updated: 2026/10/01 15:00:00 by elotana          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static int	ft_nouille(unsigned int nb)
{
	int	i;

	i = 0;
	while (nb != 0)
	{
		i++;
		nb = nb / 10;
	}
	return (i);
}

int	ft_d(unsigned int nb)
{
	int	taille;

	if (nb == 0)
	{
		write(1, "0", 1);
		return (1);
	}
	taille = ft_nouille(nb);
	if (nb > 9)
	{
		ft_d(nb / 10);
		ft_d(nb % 10);
	}
	if (nb < 10)
	{
		ft_putchar(nb + 48);
	}
	return (taille);
}

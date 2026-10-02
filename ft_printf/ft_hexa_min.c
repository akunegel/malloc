/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_hexa_min.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <akunegel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/23 19:41:57 by akunegel          #+#    #+#             */
/*   Updated: 2023/04/29 15:04:25 by akunegel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_putchar(char c)
{
	write (1, &c, 1);
	return (1);
}

int	ft_hexa_min(unsigned long long nb)
{
	char	*base;
	int		taille;

	taille = 0;
	base = "0123456789abcdef";
	if (nb > 15)
	{
		taille = ft_hexa_min(nb / 16);
		ft_hexa_min(nb % 16);
	}
	if (nb < 16)
	{
		ft_putchar(base[nb]);
	}
	return (taille + 1);
}
